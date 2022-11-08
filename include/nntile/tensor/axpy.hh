/*! @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/tensor/axpy.hh
 * AXPY for two Tensor<T>
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2022-11-08
 * */

#pragma once

#include <nntile/tensor/tensor.hh>

namespace nntile
{
namespace tensor
{

// Asynchronous tensor-wise axpy operation
template<typename T>
void axpy_async(const Tensor<T> &alpha, const Tensor<T> &src,
        const Tensor<T> &dst);

// Blocking version of tensor-wise axpy operation
template<typename T>
void axpy(const Tensor<T> &alpha, const Tensor<T> &src, const Tensor<T> &dst);

} // namespace tensor
} // namespace nntile

