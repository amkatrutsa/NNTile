/*! @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/kernel/cpu/sumnorm.hh
 * Sum and Euclidian norm of a buffer on CPU
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2022-08-09
 * */

#pragma once

#include <nntile/base_types.hh>

namespace nntile
{
namespace kernel
{
namespace cpu
{

// Compute sum and Euclidian norm along middle axis
template<typename T>
void sumnorm(Index m, Index n, Index k, const T *src, T *sumnorm)
    noexcept;

} // namespace cpu
} // namespace kernel
} // namespace nntile
