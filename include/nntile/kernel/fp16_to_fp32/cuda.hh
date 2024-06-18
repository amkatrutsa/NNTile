/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/kernel/fp16_to_fp32/cuda.hh
 * Convert fp16_t array into fp32_t array on CUDA
 *
 * @version 1.0.0
 * */

#pragma once

#include <nntile/base_types.hh>
#include <cuda_runtime.h>

namespace nntile::kernel::fp16_to_fp32
{

void cuda(cudaStream_t stream, Index nelems, const fp16_t *src, fp32_t *dst)
    noexcept;

} // namespace nntile::kernel::fp16_to_fp32

