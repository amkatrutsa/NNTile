/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/kernel/drelu/cuda.cu
 * Derivative of ReLU operation on a buffer on CUDA
 *
 * @version 1.0.0
 * */

#include "nntile/kernel/drelu/cuda.hh"

namespace nntile::kernel::drelu
{

template<typename T>
static __global__
void cuda_kernel(Index nelems, T *data)
{
    int i = threadIdx.x + blockIdx.x*blockDim.x;
    constexpr T one = 1.0, zero = 0.0;
    if(i < nelems)
    {
        T &z = data[i];
        if(z > zero)
        {
            z = one;
        }
        else
        {
            z = zero;
        }
    }
}

template<typename T>
void cuda(cudaStream_t stream, Index nelems, T *data)
    noexcept
//! Inplace derivative of ReLU operation performed on CUDA
/*! @params[in] nelems: Number of elements in a buffer
 * @params[inout] data: Buffer to apply derivative of ReLU
 * */
{
    dim3 blocks((nelems+255)/256), threads(256);
    (cuda_kernel<T>)<<<blocks, threads, 0, stream>>>(nelems, data);
}

// Explicit instantiation
template
void cuda<fp32_t>(cudaStream_t stream, Index nelems, fp32_t *data)
    noexcept;

template
void cuda<fp64_t>(cudaStream_t stream, Index nelems, fp64_t *data)
    noexcept;

} // namespace nntile::kernel::drelu
