/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/tensor/hypot.cc
 * hypot operation for Tensor<T>'s
 *
 * @version 1.0.0
 * */

#include "nntile/tensor/hypot.hh"
#include "nntile/starpu/hypot.hh"

namespace nntile::tensor
{

//! Tensor-wise hypot operation
template<typename T>
void hypot_async(scal_t alpha, const Tensor<T> &src, scal_t beta, const Tensor<T> &dst)
{
    // Check dimensions
    if(dst.ndim != src.ndim)
    {
        throw std::runtime_error("dst.ndim != src.ndim");
    }
    // Check shapes of tensors
    for(Index i = 0; i < dst.ndim; ++i)
    {
        if(dst.shape[i] != src.shape[i])
        {
            throw std::runtime_error("dst.shape[i] != src.shape[i]");
        }
        if(dst.basetile_shape[i] != src.basetile_shape[i])
        {
            throw std::runtime_error("dst.basetile_shape[i] != "
                    "src.basetile_shape[i]");
        }
    }
    // Do nothing if alpha is zero
    if(alpha == 0.0 && beta == 1.)
    {
        return;
    }
    // Apply per-tile hypot asynchronously as needed
    int mpi_rank = starpu_mpi_world_rank();
    for(Index i = 0; i < src.grid.nelems; ++i)
    {
        // Get handle for corresponding tiles of src and dst
        auto src_tile_handle = src.get_tile_handle(i);
        auto dst_tile_handle = dst.get_tile_handle(i);
        // MPI rank of the destination tile
        int dst_tile_rank = dst_tile_handle.mpi_get_rank();
        // Transfer data
        src_tile_handle.mpi_transfer(dst_tile_rank, mpi_rank);
        // Execute only on destination node
        if(mpi_rank == dst_tile_rank)
        {
            auto traits = src.get_tile_traits(i);
            starpu::hypot::submit<T>(traits.nelems, alpha, src_tile_handle,
                    beta, dst_tile_handle);
        }
        // Flush cache for the output tile on every node
        dst_tile_handle.mpi_flush();
    }
}

//! Tensor-wise hypot operation
template<typename T>
void hypot(scal_t alpha, const Tensor<T> &src, scal_t beta, const Tensor<T> &dst)
{
    hypot_async<T>(alpha, src, beta, dst);
    starpu_task_wait_for_all();
    starpu_mpi_wait_for_all(MPI_COMM_WORLD);
}

// Explicit instantiation of template
template
void hypot_async<fp32_t>(scal_t alpha, const Tensor<fp32_t> &src, scal_t beta,
        const Tensor<fp32_t> &dst);

template
void hypot_async<fp32_fast_tf32_t>(scal_t alpha, const Tensor<fp32_fast_tf32_t> &src, scal_t beta,
        const Tensor<fp32_fast_tf32_t> &dst);

template
void hypot_async<fp64_t>(scal_t alpha, const Tensor<fp64_t> &src, scal_t beta,
        const Tensor<fp64_t> &dst);

// Explicit instantiation of template
template
void hypot<fp32_t>(scal_t alpha, const Tensor<fp32_t> &src, scal_t beta,
        const Tensor<fp32_t> &dst);

template
void hypot<fp32_fast_tf32_t>(scal_t alpha, const Tensor<fp32_fast_tf32_t> &src, scal_t beta,
        const Tensor<fp32_fast_tf32_t> &dst);

template
void hypot<fp64_t>(scal_t alpha, const Tensor<fp64_t> &src, scal_t beta,
        const Tensor<fp64_t> &dst);

} // namespace nntile::tensor
