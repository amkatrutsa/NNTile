/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/kernel/scal/cuda.cu
 * Scal operation on buffers on CUDA
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-07-02
 * */

#include "nntile/kernel/scal/cuda.hh"

namespace nntile
{
namespace kernel
{
namespace scal
{

template<typename T>
static __global__
void cuda_kernel(Index nelems, T alpha, const T* src, T* dst)
//! Set one buffer as a scaled version of another
/*! Performs the followin operation:
 *      dst[i] = alpha * src[i]
 *
 * @param[in] nelems: Size of the src and dst tensors
 * @param[in] alpha: Scalar multiplier for the src tensor
 * @param[in] src: Source tensor
 * @param[out] dst: Destination of the scal operation. Input values are
 *      ignored, its content is overwritten on exit.
 * */
{
    int i = threadIdx.x + blockIdx.x*blockDim.x;
    if(i < nelems)
    {
        dst[i] = alpha * src[i];
    }
}

template<typename T>
void cuda(cudaStream_t stream, Index nelems, T alpha, const T *src, T *dst)
    noexcept
//! Set one buffer as a scaled version of another
/*! Performs the followin operation:
 *      dst[i] = alpha * src[i]
 *
 * @param[in] nelems: Size of the src and dst tensors
 * @param[in] alpha: Scalar multiplier for the src tensor
 * @param[in] src: Source tensor
 * @param[out] dst: Destination of the scal operation. Input values are
 *      ignored, its content is overwritten on exit.
 * */
{
    dim3 blocks((nelems+255)/256), threads(256);
    (cuda_kernel<T>)<<<blocks, threads, 0, stream>>>(nelems, alpha, src, dst);
}

// Explicit instantiation
template
void cuda<fp32_t>(cudaStream_t stream, Index nelems, fp32_t alpha,
        const fp32_t *src, fp32_t *dst)
    noexcept;

template
void cuda<fp64_t>(cudaStream_t stream, Index nelems, fp64_t alpha,
        const fp64_t *src, fp64_t *dst)
    noexcept;

} // namespace scal
} // namespace kernel
} // namespace nntile

