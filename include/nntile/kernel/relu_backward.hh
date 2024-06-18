/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/kernel/relu_backward.hh
 * Backward ReLU low-level kernels
 *
 * @version 1.0.0
 * */

#pragma once

#include <nntile/kernel/relu_backward/cpu.hh>
#include <nntile/defs.h>
#ifdef NNTILE_USE_CUDA
#include <nntile/kernel/relu_backward/cuda.hh>
#endif // NNTILE_USE_CUDA

//! @namespace nntile::kernel::relu
/*! Low-level implementations of backward ReLU operation
 * */
namespace nntile::kernel::relu
{

} // namespace nntile::kernel::relu

