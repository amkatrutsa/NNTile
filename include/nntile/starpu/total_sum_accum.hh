/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/starpu/total_sum_accum.hh
 * Total sum accumulating for StarPU buffer
 *
 * @version 1.0.0
 * */

#pragma once

#include <nntile/base_types.hh>
#include <nntile/starpu/config.hh>

namespace nntile::starpu::total_sum_accum
{

//! Structure for arguments
template<typename T>
struct args_t
{
    T alpha;
    Index n_labels;
    Index n_outputs;
};

// Total sum accumulating of StarPU buffer on CPU
template<typename T>
void cpu(void *buffers[], void *cl_args)
    noexcept;

#ifdef NNTILE_USE_CUDA
// Total sum accumulating of StarPU buffer on CUDA
// template<typename T>
// void cuda(void *buffers[], void *cl_args)
//     noexcept;
#endif // NNTILE_USE_CUDA

extern Codelet codelet_fp32, codelet_fp64;

template<typename T>
constexpr Codelet *codelet()
{
    throw std::runtime_error("Non-supported type");
    return nullptr;
}

template<>
constexpr Codelet *codelet<fp32_t>()
{
    return &codelet_fp32;
}

template<>
constexpr Codelet *codelet<fp64_t>()
{
    return &codelet_fp64;
}

void init();

void restrict_where(uint32_t where);

void restore_where();

template<typename T>
void submit(T alpha, Index n_labels, Index n_outputs, Handle logsumexp, Handle src,
        Handle class_labels, Handle val);

} // namespace nntile::starpu::total_sum_accum

