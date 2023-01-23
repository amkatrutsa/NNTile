/*! @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file wrappers/python/nntile/nntile_core.cc
 * Extension module with NNTile wrappers
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-01-23
 * */

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <nntile.hh>
#include <sstream>
#include <cstring>

using namespace nntile;
namespace py = pybind11;

// Extend (sub)module with nntile::starpu functionality
void def_mod_starpu(py::module_ &m)
{
    using namespace nntile::starpu;
    py::class_<Config>(m, "Config").
        def(py::init<int, int, int>());
    m.def("init", init);
    m.def("shutdown", starpu_shutdown);
    m.def("pause", starpu_pause);
    m.def("resume", starpu_resume);
    m.def("wait_for_all", [](){starpu_task_wait_for_all;
            starpu_mpi_wait_for_all(MPI_COMM_WORLD);});
}

// numpy.ndarray -> Tile
template<typename T>
void tile_from_array(const tile::Tile<T> &tile,
        const py::array_t<T, py::array::f_style | py::array::forcecast> &array)
{
    if(tile.ndim != array.ndim())
    {
        throw std::runtime_error("tile.ndim != array.ndim()");
    }
    for(Index i = 0; i < tile.ndim; ++i)
    {
        if(array.shape()[i] != tile.shape[i])
        {
            throw std::runtime_error("array.shape()[i] != tile.shape[i]");
        }
    }
    // Acquire tile and copy data
    auto tile_local = tile.acquire(STARPU_W);
    std::memcpy(tile_local.get_ptr(), array.data(),
            tile.nelems*sizeof(T));
    tile_local.release();
}

// Tile -> numpy.ndarray
template<typename T>
void tile_to_array(const tile::Tile<T> &tile,
        py::array_t<T, py::array::f_style> &array)
{
    if(tile.ndim != array.ndim())
    {
        throw std::runtime_error("tile.ndim != array.ndim()");
    }
    for(Index i = 0; i < tile.ndim; ++i)
    {
        if(array.shape()[i] != tile.shape[i])
        {
            throw std::runtime_error("array.shape()[i] != tile.shape[i]");
        }
    }
    // Acquire tile and copy data
    auto tile_local = tile.acquire(STARPU_R);
    std::memcpy(array.mutable_data(), tile_local.get_ptr(),
            tile.nelems*sizeof(T));
    tile_local.release();
}

// Extend (sub)module with nntile::tile::Tile<T>
template<typename T>
void def_class_tile(py::module_ &m, const char *name)
{
    using namespace nntile::tile;
    py::class_<Tile<T>, TileTraits>(m, name, py::multiple_inheritance()).
        def(py::init<const TileTraits &>()).
        def("unregister", &Tile<T>::unregister).
        def("from_array", tile_from_array<T>).
        def("to_array", tile_to_array<T>);
    m.def("tile_from_array", tile_from_array<T>);
    m.def("tile_to_array", tile_to_array<T>);
}

// Extend (sub)module with nntile::tile functionality
void def_mod_tile(py::module_ &m)
{
    using namespace nntile::tile;
    // Define wrapper for the Class
    py::class_<TileTraits>(m, "TileTraits").
        // Constructor
        def(py::init<const std::vector<Index> &>()).
        // __repr__ function for print(object)
        def("__repr__", [](const TileTraits &data){
                std::stringstream stream;
                stream << data;
                return stream.str();}).
        // Number of dimensions
        def_readonly("ndim", &TileTraits::ndim).
        // Shape of a tile
        def_readonly("shape", &TileTraits::shape).
        // Number of elements of a tile
        def_readonly("nelems", &TileTraits::nelems);
    // Define wrappers for Tile<T>
    def_class_tile<fp32_t>(m, "Tile_fp32");
    def_class_tile<fp64_t>(m, "Tile_fp64");
}

// Extend (sub)module with nntile::tensor::Tensor<T>
template<typename T>
void def_class_tensor(py::module_ &m, const char *name)
{
    using namespace nntile::tensor;
    py::class_<Tensor<T>, TensorTraits>(m, name, py::multiple_inheritance()).
        def(py::init<const TensorTraits &, const std::vector<int> &,
                starpu_mpi_tag_t &>()).
        def_readonly("next_tag", &Tensor<T>::next_tag);
//        def("unregister", &Tensor<T>::unregister).
//        def("from_array", tensor_from_array<T>);
//        def("to_array", tensor_to_array<T>);
//    m.def("tensor_to_array", tensor_to_array<T>);
//    m.def("tensor_from_array", tensor_to_array<T>);
}

// Extend (sub)module with nntile::tensor::distributions functionality
void def_tensor_distributions(py::module_ &m)
{
    using namespace nntile::tensor::distributions;
    m.def("block_cyclic", &block_cyclic);
}

// Extend (sub)module with nntile::tensor functionality
void def_mod_tensor(py::module_ &m)
{
    using namespace nntile::tensor;
    // Define wrapper for TensorTraits
    py::class_<TensorTraits, tile::TileTraits>(m, "TensorTraits",
            py::multiple_inheritance()).
        // Constructor
        def(py::init<const std::vector<Index> &,
                const std::vector<Index> &>()).
        // __repr__ function for print(object)
        def("__repr__", [](const TensorTraits &data){
                std::stringstream stream;
                stream << data;
                return stream.str();}).
        // Shape of corresponding tile
        def("get_tile_shape", &TensorTraits::get_tile_shape).
        // Shape of a grid
        def("get_grid_shape", [](const TensorTraits &data){
                return data.grid.shape;});
    // Define wrappers for Tensor<T>
    def_class_tensor<fp32_t>(m, "Tensor_fp32");
    def_class_tensor<fp64_t>(m, "Tensor_fp64");
    // Add tensor.distributions submodule
    auto distributions = m.def_submodule("distributions");
    def_tensor_distributions(distributions);
}

// Main extension module with all wrappers
PYBIND11_MODULE(nntile_core, m)
{
    // Add starpu submodule
    auto starpu = m.def_submodule("starpu");
    def_mod_starpu(starpu);
    // Add tile submodule
    auto tile = m.def_submodule("tile");
    def_mod_tile(tile);
    // Add tensor submodule
    auto tensor = m.def_submodule("tensor");
    def_mod_tensor(tensor);
}

