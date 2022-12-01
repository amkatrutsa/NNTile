/*! @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/tensor/sumnorm.cc
 * Sum and Euclidian norm of Tensor<T> along axis
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2022-12-01
 * */

#include "nntile/tensor/sumnorm.hh"
#include "nntile/starpu/sumnorm.hh"
#include "nntile/starpu/clear.hh"

namespace nntile
{
namespace tensor
{

//! Compute sum of elements and Euclidian norm of slices along given axis
template<typename T>
void sumnorm_async(const Tensor<T> &src, const Tensor<T> &dst, Index axis)
{
    // Check dimensions
    if(src.ndim != dst.ndim)
    {
        throw std::runtime_error("src.ndim != dst.ndim");
    }
    // Treat special case of src.ndim=0
    if(src.ndim == 0)
    {
        throw std::runtime_error("Scalar input makes no sense");
    }
    // Check axis
    if(axis < 0)
    {
        throw std::runtime_error("axis < 0");
    }
    if(axis >= src.ndim)
    {
        throw std::runtime_error("axis >= src.ndim");
    }
    // Check shapes of src and dst
    if(dst.shape[0] != 2)
    {
        throw std::runtime_error("dst.shape[0] != 2");
    }
    if(dst.basetile_shape[0] != 2)
    {
        throw std::runtime_error("dst.basetile_shape[0] != 2");
    }
    for(Index i = 0; i < axis; ++i)
    {
        if(src.shape[i] != dst.shape[i+1])
        {
            throw std::runtime_error("src.shape[i] != dst.shape[i+1]");
        }
        if(src.basetile_shape[i] != dst.basetile_shape[i+1])
        {
            throw std::runtime_error("src.basetile_shape[i] != "
                    "dst.basetile_shape[i+1]");
        }
    }
    for(Index i = axis+1; i < src.ndim; ++i)
    {
        if(src.shape[i] != dst.shape[i])
        {
            throw std::runtime_error("src.shape[i] != dst.shape[i]");
        }
        if(src.basetile_shape[i] != dst.basetile_shape[i])
        {
            throw std::runtime_error("src.basetile_shape[i] != "
                    "dst.basetile_shape[i]");
        }
    }
    // Do actual calculations
    int mpi_rank = starpu_mpi_world_rank();
    int ret;
    Index ndim = src.ndim;
    for(Index i = 0; i < dst.grid.nelems; ++i)
    {
        // Clean up destination tile on dest node
        auto dst_tile_handle = dst.get_tile_handle(i);
        int dst_tile_rank = dst_tile_handle.mpi_get_rank();
        if(mpi_rank == dst_tile_rank)
        {
            starpu::clear::submit(dst_tile_handle);
        }
        // Obtain indices of applicable source tiles
        auto dst_tile_index = dst.grid.linear_to_index(i);
        std::vector<Index> src_tile_index(src.ndim);
        for(Index j = 0; j < axis; ++j)
        {
            src_tile_index[j] = dst_tile_index[j+1];
        }
        src_tile_index[axis] = 0;
        for(Index j = axis+1; j < src.ndim; ++j)
        {
            src_tile_index[j] = dst_tile_index[j];
        }
        // Launch kernel for each appropriate source tile
        auto dst_tile_traits = dst.get_tile_traits(i);
        for(Index j = 0; j < src.grid.shape[axis]; ++j)
        {
            src_tile_index[axis] = j;
            Index src_tile_offset = src.grid.index_to_linear(src_tile_index);
            auto src_tile_handle = src.get_tile_handle(src_tile_offset);
            int src_tile_rank = src_tile_handle.mpi_get_rank();
            // Transfer data
            src_tile_handle.mpi_transfer(dst_tile_rank, mpi_rank);
            // Execute on destination node
            if(mpi_rank == dst_tile_rank)
            {
                // Get sizes
                auto src_tile_traits = src.get_tile_traits(src_tile_offset);
                Index m, n, k;
                if(axis == 0)
                {
                    m = 1;
                    n = dst_tile_traits.nelems / 2;
                    k = src_tile_traits.shape[0];
                }
                else if(axis == ndim-1)
                {
                    m = dst_tile_traits.nelems / 2;
                    n = 1;
                    k = src_tile_traits.shape[axis];
                }
                else
                {
                    m = src_tile_traits.stride[axis];
                    n = src_tile_traits.matrix_shape[axis+1][1];
                    k = src_tile_traits.shape[axis];
                }
                // Insert task
                starpu::sumnorm::submit<T>(m, n, k, src_tile_handle,
                        dst_tile_handle);
            }
        }
        // Flush cache for the output tile on every node
        dst_tile_handle.mpi_flush();
    }
}


template<typename T>
void sumnorm(const Tensor<T> &src, const Tensor<T> &dst, Index axis)
{
    sumnorm_async<T>(src, dst, axis);
    starpu_task_wait_for_all();
    starpu_mpi_wait_for_all(MPI_COMM_WORLD);
}

// Explicit instantiation
template
void sumnorm_async<fp32_t>(const Tensor<fp32_t> &src,
        const Tensor<fp32_t> &dst, Index axis);

template
void sumnorm_async<fp64_t>(const Tensor<fp64_t> &src,
        const Tensor<fp64_t> &dst, Index axis);

// Explicit instantiation
template
void sumnorm<fp32_t>(const Tensor<fp32_t> &src, const Tensor<fp32_t> &dst,
        Index axis);

template
void sumnorm<fp64_t>(const Tensor<fp64_t> &src, const Tensor<fp64_t> &dst,
        Index axis);

} // namespace tensor
} // namespace nntile

