/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/kernel/normalize/cpu.cc
 * Normalize operation for a buffer on CPU
 *
 * @version 1.0.0
 * */

#include "nntile/kernel/normalize/cpu.hh"
#include <cmath>

namespace nntile::kernel::normalize
{

template<typename T>
void cpu(Index m, Index n, Index k, Index l, T eps, const T *gamma,
        const T *beta, const T *sumnorm, T *dst)
    noexcept
//! Renormalize buffer along middle axis
/*! Provided m-by-k-by-n output array dst is renormalized along second axis
 * with k elements. The following operations is applied:
 *      dst[i, :, j] := (dst[i, :, j]-mean(i, j)) / sqrt(var(i, j)+eps)
 *          * gamma + beta
 * where mean and var functions are computed as follows:
 *      mean(i, j) = sumnorm[0, i, j] / l
 *      var(i, j) = sumnorm[1, i, j]^2/l - mean(i,j)^2
 *
 * @param[in] m: Size of the first mode of dst and sumnorm arrays
 * @param[in] n: Size of the last mode of dst and sumnorm arrays
 * @param[in] k: Size of the middle mode of dst array
 * @param[in] l: Number of elements used to calculate sum and Euclidean norm
 * @param[in] eps: Regularization parameter for variance. eps > 0
 * @param[in] gamma: Deviation for the renormalized output
 * @param[in] beta: Mean value for the renormalized output
 * @param[in] sumnorm: Sums and norms of slices
 * @param[in] dst: Contiguous output array
 * */
{
    Index dst_offset = 0;
    constexpr T one = 1;
    const T invl = one / T(l);
    const T rinvl = std::sqrt(invl);
    const T reps = std::sqrt(eps);
    // Outer loop by the last mode of dst and sumnorm arrays
    for(Index i2 = 0; i2 < n; ++i2)
    {
        // Middle loop by the middle mode of dst array
        for(Index i1 = 0; i1 < k; ++i1)
        {
            Index src_offset = 2 * m * i2;
            // Inner loop by the first mode of dst and sumnorm arrays
            for(Index i0 = 0; i0 < m; ++i0)
            {
                // Value-to-update
                T &val = dst[dst_offset];
                // Corresponding mean and root-mean-square
                const T sum = sumnorm[src_offset];
                const T mean = sum * invl;
                const T norm = sumnorm[src_offset+1];
                const T rms = norm * rinvl;
                // Deviation=sqrt(rms*rms-mean*mean+reps*reps)
                T dev;
                // Although in theory tmp<=1 it is not always true in practice
                // due presence of rounding errors
                T tmp = std::fabs(mean) / rms;
                // Check if rounding errors broke theoretical invariant
                if(tmp >= one)
                {
                    dev = reps;
                }
                else if(rms > reps)
                {
                    T ssq = one - tmp*tmp;
                    T tmp2 = reps / rms;
                    ssq += tmp2*tmp2;
                    dev = rms * std::sqrt(ssq);
                }
                else
                {
                    T ssq = one - tmp*tmp;
                    T tmp2 = rms / reps;
                    ssq *= tmp2 * tmp2;
                    ssq += one;
                    dev = reps * std::sqrt(ssq);
                }
                // Normalization
                val = (val-mean) / dev;
                // Renormalization
                val = val*gamma[0] + beta[0];
                // Update pointers
                ++dst_offset;
                src_offset += 2;
            }
        }
    }
}

// Explicit instantiation
template
void cpu<fp32_t>(Index m, Index n, Index k, Index l, fp32_t eps,
        const fp32_t *gamma, const fp32_t *beta, const fp32_t *sumnorm,
        fp32_t *dst)
    noexcept;

template
void cpu<fp64_t>(Index m, Index n, Index k, Index l, fp64_t eps,
        const fp64_t *gamma, const fp64_t *beta, const fp64_t *sumnorm,
        fp64_t *dst)
    noexcept;

} // namespace nntile::kernel::normalize
