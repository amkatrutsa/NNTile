/*! @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file tests/kernel/normalize.cc
 * Normalize operation for a buffer on CPU
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2022-10-25
 * */

#include "nntile/kernel/normalize.hh"
#include "../testing.hh"
#include <vector>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <iostream>

using namespace nntile;
using namespace nntile::kernel::normalize;

#ifdef NNTILE_USE_CUDA
template<typename T>
void run_cuda(Index m, Index n, Index k, Index l, T eps, T gamma, T beta,
        const std::vector<T> &sumnorm, std::vector<T> &dst)
{
    // Copy to device
    T *dev_sumnorm, *dev_dst, *dev_gamma, *dev_beta;
    cudaError_t cuda_err = cudaMalloc(&dev_sumnorm, sizeof(T)*2*m*n);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMalloc(&dev_dst, sizeof(T)*m*n*k);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMalloc(&dev_gamma, sizeof(T));
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMalloc(&dev_beta, sizeof(T));
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMemcpy(dev_sumnorm, &sumnorm[0], sizeof(T)*2*m*n,
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMemcpy(dev_dst, &dst[0], sizeof(T)*m*n*k,
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMemcpy(dev_gamma, &gamma, sizeof(T),
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMemcpy(dev_beta, &beta, sizeof(T),
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Init stream
    cudaStream_t stream;
    cuda_err = cudaStreamCreate(&stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Launch low-level kernel
    cuda<T>(stream, m, n, k, l, eps, dev_gamma, dev_beta,
            dev_sumnorm, dev_dst);
    // Wait for result and destroy stream
    cuda_err = cudaStreamSynchronize(stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaStreamDestroy(stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Copy result and deallocate device memory
    cuda_err = cudaMemcpy(&dst[0], dev_dst, sizeof(T)*m*n*k,
            cudaMemcpyDeviceToHost);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_sumnorm);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_dst);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_gamma);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_beta);
    TEST_ASSERT(cuda_err == cudaSuccess);
}
#endif // NNTILE_USE_CUDA

// Templated validation
template<typename T>
void validate(Index m, Index n, Index k, Index l, T eps, T gamma, T beta)
{
    constexpr T epsilon = std::numeric_limits<T>::epsilon();
    // Init test input
    std::vector<T> sumnorm(2*m*n), dst(m*n*k);
    for(Index i0 = 0; i0 < m; ++i0)
    {
        for(Index i1 = 0; i1 < n; ++i1)
        {
            for(Index i2 = 0; i2 < k; ++i2)
            {
                dst[(i1*k+i2)*m+i0] = T(i0+i1+i2) / T{10};
            }
        }
    }
    for(Index i0 = 0; i0 < m; ++i0)
    {
        for(Index i1 = 0; i1 < n; ++i1)
        {
            T avg = T(i0+i1) / T{10};
            sumnorm[2*(i1*m+i0)] = avg * T(l);
            sumnorm[2*(i1*m+i0)+1] = std::sqrt((avg*avg+T{1}) * T(l));
        }
    }
    std::vector<T> dst_save(dst);
    // Check low-level kernel
    std::cout << "Run kernel::normalize::cpu<T>\n";
    cpu<T>(m, n, k, l, eps, &gamma, &beta, &sumnorm[0], &dst[0]);
    for(Index i0 = 0; i0 < m; ++i0)
    {
        for(Index i1 = 0; i1 < n; ++i1)
        {
            for(Index i2 = 0; i2 < k; ++i2)
            {
                T val = dst[(i1*k+i2)*m+i0];
                T val_ref = T(i2)/T{10}/std::sqrt(T{1}+eps)*gamma + beta;
                if(val_ref == T{0})
                {
                    TEST_ASSERT(std::abs(val) <= 50*eps);
                }
                else
                {
                    TEST_ASSERT(std::abs(val/val_ref-T{1}) < 50*epsilon);
                }
            }
        }
    }
    std::cout << "OK: kernel::normalize::cpu<T>\n";
#ifdef NNTILE_USE_CUDA
    // Check low-level CUDA kernel
    dst = dst_save;
    std::cout << "Run kernel::normalize::cuda<T>\n";
    run_cuda<T>(m, n, k, l, eps, gamma, beta, sumnorm, dst);
    for(Index i0 = 0; i0 < m; ++i0)
    {
        for(Index i1 = 0; i1 < n; ++i1)
        {
            for(Index i2 = 0; i2 < k; ++i2)
            {
                T val = dst[(i1*k+i2)*m+i0];
                T val_ref = T(i2)/T{10}/std::sqrt(T{1}+eps)*gamma + beta;
                if(val_ref == T{0})
                {
                    TEST_ASSERT(std::abs(val) <= 10*eps);
                }
                else
                {
                    TEST_ASSERT(std::abs(val/val_ref-T{1}) < 50*epsilon);
                }
            }
        }
    }
    std::cout << "OK: kernel::normalize::cuda<T>\n";
#endif // NNTILE_USE_CUDA
}

int main(int argc, char **argv)
{
    fp64_t eps[3] = {0.0, 1.0, 1111.1};
    fp64_t gamma[3] = {0.0, 1.0, 3.3};
    fp64_t beta[3] = {0.0, 1.1, -2.2};
    for(Index i = 0 ; i < sizeof(eps)/sizeof(eps[0]); ++i)
    {
        for(Index j = 0 ; j < sizeof(gamma)/sizeof(gamma[0]); ++j)
        {
            for(Index k = 0 ; k < sizeof(beta)/sizeof(beta[0]); ++k)
            {
                validate<fp32_t>(1, 9, 11, 22, eps[i], gamma[j], beta[k]);
                validate<fp32_t>(8, 1, 11, 22, eps[i], gamma[j], beta[k]);
                validate<fp32_t>(8, 9, 1, 22, eps[i], gamma[j], beta[k]);
                validate<fp64_t>(1, 9, 11, 22, eps[i], gamma[j], beta[k]);
                validate<fp64_t>(8, 1, 11, 22, eps[i], gamma[j], beta[k]);
                validate<fp64_t>(8, 9, 1, 22, eps[i], gamma[j], beta[k]);
            }
        }
    }
    return 0;
}

