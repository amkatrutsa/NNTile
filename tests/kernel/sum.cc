/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file tests/kernel/sum.cc
 * Sum of a buffer on CPU
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @author Konstantin Sozykin
 * @date 2023-02-27
 * */

#include "nntile/kernel/sum.hh"
#include "../testing.hh"
#include <vector>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <iostream>

using namespace nntile;
using namespace nntile::kernel::sum;

#ifdef NNTILE_USE_CUDA
template<typename T>
void run_cuda(Index m, Index n, Index k, const std::vector<T> &src,
        std::vector<T> &sum_dst)
{
    // Copy to device
    T *dev_src, *dev_sum_dst;
    cudaError_t cuda_err = cudaMalloc(&dev_src, sizeof(T)*m*n*k);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMalloc(&dev_sum_dst, sizeof(T)*m*n);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMemcpy(dev_src, &src[0], sizeof(T)*m*n*k,
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMemcpy(dev_sum_dst, &sum_dst[0], sizeof(T)*m*n,
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Init stream
    cudaStream_t stream;
    cuda_err = cudaStreamCreate(&stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Launch low-level kernel
    cuda<T>(stream, m, n, k, dev_src, dev_sum_dst);
    cuda_err = cudaStreamSynchronize(stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Copy result and deallocate device memory
    cuda_err = cudaMemcpy(&sum_dst[0], dev_sum_dst, sizeof(T)*m*n,
            cudaMemcpyDeviceToHost);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_src);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_sum_dst);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaStreamDestroy(stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
}
#endif // NNTILE_USE_CUDA

// Templated validation
template<typename T>
void validate(Index m, Index n, Index k)
{
    constexpr T eps = std::numeric_limits<T>::epsilon();
    // Init test input
    std::vector<T> src(m*n*k), sum_dst(m*n);
    for(Index i0 = 0; i0 < m; ++i0)
    {
        for(Index i1 = 0; i1 < n; ++i1)
        {
            for(Index i2 = 0; i2 < k; ++i2)
            {
                src[(i1*k+i2)*m+i0] = T(i0+i1+i2) / T{10};
            }
        }
    }
    
    // Check low-level kernel
    std::cout << "Run kernel::sum::cpu<T>\n";
    cpu<T>(m, n, k, &src[0], &sum_dst[0]);
    for(Index i0 = 0; i0 < m; ++i0)
    {
        for(Index i1 = 0; i1 < n; ++i1)
        {
            Index a = i0 + i1;
            T sum_ref = k * (2*a+k-1) / 2 / T{10};
            T sum = sum_dst[i1*m+i0];
            if(sum_ref == T{0})
            {
                TEST_ASSERT(std::abs(sum) <= 10*eps);
            }
            else
            {
                TEST_ASSERT(std::abs(sum/sum_ref-T{1}) <= 10*eps);
            }
            
        }
    }
    std::cout << "OK: kernel::sum::cpu<T>\n";
    
#ifdef NNTILE_USE_CUDA
    // Check low-level CUDA kernel
    std::vector<T> sum_copy(sum_dst);
    std::vector<T> sum_cuda(m*n);
    std::cout << "Run kernel::sum::cuda<T>\n";
    run_cuda<T>(m, n, k, src, sum_cuda);
    for(Index i0 = 0; i0 < m; ++i0)
    {
        for(Index i1 = 0; i1 < n; ++i1)
        {
            Index i = (i1*m+i0);
            TEST_ASSERT(std::abs(sum_cuda[i] - sum_copy[i]) <= eps); 
            std::cout << "sum_cuda[i] " << sum_cuda[i] << " sum_copy[i] " << sum_copy[i] << std::endl;
        }
    }
    
    for(Index i0 = 0; i0 < m; ++i0)
    {
        for(Index i1 = 0; i1 < n; ++i1)
        {
            Index i = (i1*m+i0);
            if(sum_copy[i] == T{0})
            {
                TEST_ASSERT(sum_cuda[i] == T{0});   
            }
            else
            {   
                TEST_ASSERT(std::abs(sum_cuda[i]/sum_copy[i]-T{1}) <= 10*eps); 
            }
            
        }
    }
    std::cout << "OK: kernel::sum::cuda<T>\n";
#endif // NNTILE_USE_CUDA
}


int main(int argc, char **argv)
{
    validate<fp32_t>(1, 9, 10);
    validate<fp32_t>(8, 9, 1);
    validate<fp32_t>(8, 1, 10);
    validate<fp32_t>(4, 7, 8);
    validate<fp64_t>(1, 9, 10);
    validate<fp64_t>(8, 9, 1);
    validate<fp64_t>(8, 1, 10);
    validate<fp64_t>(4, 7, 8);
    return 0;
}