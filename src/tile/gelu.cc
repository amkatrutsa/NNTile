/*! @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/tile/gelu.cc
 * GeLU operation for Tile<T>
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2022-10-26
 * */

#include "nntile/tile/gelu.hh"
#include "nntile/starpu/gelu.hh"

namespace nntile
{
namespace tile
{

//! Asynchronous tile-wise GeLU operation
/*! @param[inout] A: Tile for the element-wise GeLU operation
 * */
template<typename T>
void gelu_async(const Tile<T> &A)
{
    // Submit task without any arguments checked
    starpu::gelu::submit<T>(A.nelems, A);
}

//! Blocking version of tile-wise GeLU operation
/*! @param[inout] A: Tile for the element-wise GeLU operation
 * */
template<typename T>
void gelu(const Tile<T> &A)
{
    gelu_async<T>(A);
    starpu_task_wait_for_all();
}

// Explicit instantiation
template
void gelu<fp32_t>(const Tile<fp32_t> &A);

template
void gelu<fp64_t>(const Tile<fp64_t> &A);

} // namespace tile
} // namespace nntile

