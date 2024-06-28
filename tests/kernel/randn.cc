/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file tests/kernel/randn.cc
 * Randn operation on CPU
 *
 * @version 1.0.0
 * */

#include "nntile/kernel/randn.hh"
#include "../external/random.h" // external
#include "../testing.hh"
#include <array>
#include <vector>
#include <stdexcept>
#include <limits>
#include <iostream>
#include <cmath>

using namespace nntile;
using namespace nntile::kernel::randn;

static inline fp32_t chameleon_randn(unsigned long long &seed, fp32_t mean,
        fp32_t stddev)
{
    constexpr fp32_t two=2.0, twopi=6.2831853071795864769252867663;
    fp32_t t1 = CORE_slaran(&seed);
    fp32_t t2 = CORE_slaran(&seed) * twopi;
    fp32_t t3 = std::sqrt(-two*std::log(t1)) * std::cos(t2);
    return stddev*t3 + mean;
}

static inline fp64_t chameleon_randn(unsigned long long &seed, fp64_t mean,
        fp64_t stddev)
{
    constexpr fp64_t two=2.0, twopi=6.2831853071795864769252867663;
    fp64_t t1 = CORE_dlaran(&seed);
    fp64_t t2 = CORE_dlaran(&seed) * twopi;
    fp64_t t3 = std::sqrt(-two*std::log(t1)) * std::cos(t2);
    return stddev*t3 + mean;
}

template<typename T>
void validate_empty_shape()
{
    // Set default values for tests
    T mean = 0, stddev = 1;
    unsigned long long seed = CORE_rnd64_jump(1000, -1);
    // Init reference array
    T data_ref;
    unsigned long long seed2 = seed;
    data_ref = chameleon_randn(seed2, mean, stddev);
    // Run kernel
    T data;
    std::cout << "Run kernel::randn::cpu_ndim0<T>\n";
    cpu_ndim0<T>(seed, mean, stddev, &data);
    // Check if the result is the same as the reference one
    TEST_ASSERT(data == data_ref);
    std::cout << "OK: kernel::randn::cpu_ndim0<T>\n";
    // Run kernel with a different seed that shall generate different result
    seed2 = seed + std::numeric_limits<unsigned long long>::max()/2;
    // Launch kernel
    std::cout << "Run kernel::randn::cpu_ndim0<T>\n";
    cpu_ndim0<T>(seed2, mean, stddev, &data);
    // Check if result is different
    TEST_ASSERT(data != data_ref);
    std::cout << "OK: kernel::randn::cpu_ndim0<T>\n";
    // Run kernel with a different mean
    T mean2 = mean + T{1};
    // Launch kernel
    std::cout << "Run kernel::randn::cpu_ndim0<T>\n";
    cpu_ndim0(seed, mean2, stddev, &data);
    // Check if result is different for the first element
    TEST_ASSERT(data != data_ref);
    std::cout << "OK: kernel::randn::cpu_ndim0<T>\n";
    // Run kernel with a different stddev
    T stddev2 = stddev + T{1};
    // Launch kernel
    std::cout << "Run kernel::randn::cpu_ndim0<T>\n";
    cpu_ndim0<T>(seed, mean, stddev2, &data);
    // Check if result is different for the first element
    TEST_ASSERT(data != data_ref);
    std::cout << "OK: kernel::randn::cpu_ndim0<T>\n";
}

// Check generation of a full contiguous array, which actually checks
// parameters seed, mean and stddev of randn() function
template<typename T, std::size_t NDIM>
void validate_full(std::array<Index, NDIM> shape)
{
    // Set default values for tests
    T mean = 0, stddev = 1;
    unsigned long long seed = CORE_rnd64_jump(1000, -1);
    // Init strides
    std::array<Index, NDIM> stride, start, tmp_index;
    stride[0] = 1;
    start[0] = 0;
    for(Index i = 1; i < NDIM; ++i)
    {
        stride[i] = stride[i-1] * shape[i-1];
        start[i] = 0;
    }
    Index nelems = stride[NDIM-1] * shape[NDIM-1];
    // Init reference array
    std::vector<T> data_ref(nelems);
    unsigned long long seed2 = seed;
    for(Index i = 0; i < nelems; ++i)
    {
        data_ref[i] = chameleon_randn(seed2, mean, stddev);
    }
    // Run kernel
    std::vector<T> data(nelems);
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu<T>(NDIM, nelems, seed, mean, stddev, &start[0], &shape[0],
            &shape[0], &data[0], &stride[0], &tmp_index[0]);
    // Check if the result is the same as the reference one
    for(Index i = 0; i < nelems; ++i)
    {
        TEST_ASSERT(data[i] == data_ref[i]);
    }
    std::cout << "OK: kernel::randn::cpu<T>\n";
    // Run kernel with a different seed that shall generate different result
    seed2 = seed + std::numeric_limits<unsigned long long>::max()/2;
    // Launch kernel
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu<T>(NDIM, nelems, seed2, mean, stddev, &start[0], &shape[0],
            &shape[0], &data[0], &stride[0], &tmp_index[0]);
    // Check if result is different for the first element
    TEST_ASSERT(data[0] != data_ref[0]);
    std::cout << "OK: kernel::randn::cpu<T>\n";
    // Run kernel with a different mean
    T mean2 = mean + T{1};
    // Launch kernel
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu(NDIM, nelems, seed, mean2, stddev, &start[0], &shape[0],
            &shape[0], &data[0], &stride[0], &tmp_index[0]);
    // Check if result is different for the first element
    TEST_ASSERT(data[0] != data_ref[0]);
    std::cout << "OK: kernel::randn::cpu<T>\n";
    // Run kernel with a different stddev
    T stddev2 = stddev + T{1};
    // Launch kernel
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu<T>(NDIM, nelems, seed, mean, stddev2, &start[0], &shape[0],
            &shape[0], &data[0], &stride[0], &tmp_index[0]);
    // Check if result is different for the first element
    TEST_ASSERT(data[0] != data_ref[0])
    std::cout << "OK: kernel::randn::cpu<T>\n";
}

// Check generation of a full contiguous 0-dimensional array
template<typename T>
void validate_full(std::array<Index, 0> shape_)
{
    // Set default values for tests
    T mean = 0, stddev = 1;
    unsigned long long seed = CORE_rnd64_jump(1000, -1);
    // 0-dimensional arrays are not referenced, so we just init them with null
    // pointers
    Index *start = nullptr, *shape = nullptr, *stride = nullptr,
          *tmp_index = nullptr;
    // Init nelems
    Index nelems = 1;
    // Init reference array
    unsigned long long seed2 = seed;
    T data_ref = chameleon_randn(seed2, mean, stddev);
    // Run kernel
    T data;
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu<T>(0, nelems, seed, mean, stddev, start, shape, shape, &data,
            stride, tmp_index);
    // Check if the result is the same as the reference one
    TEST_ASSERT(data == data_ref);
    std::cout << "OK: kernel::randn::cpu<T>\n";
    // Run kernel with a different seed that shall generate different result
    seed2 = seed + std::numeric_limits<unsigned long long>::max()/2;
    // Launch kernel
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu<T>(0, nelems, seed2, mean, stddev, start, shape, shape, &data,
            stride, tmp_index);
    // Check if result is different for the first element
    TEST_ASSERT(data != data_ref);
    std::cout << "OK: kernel::randn::cpu<T>\n";
    // Run kernel with a different mean
    T mean2 = mean + T{1};
    // Launch kernel
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu<T>(0, nelems, seed, mean2, stddev, start, shape, shape, &data,
            stride, tmp_index);
    // Check if result is different for the first element
    TEST_ASSERT(data != data_ref);
    std::cout << "OK: kernel::randn::cpu<T>\n";
    // Run kernel with a different stddev
    T stddev2 = stddev + T{1};
    // Launch kernel
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu<T>(0, nelems, seed, mean, stddev2, start, shape, shape, &data,
            stride, tmp_index);
    // Check if result is different for the first element
    TEST_ASSERT(data != data_ref);
    std::cout << "OK: kernel::randn::cpu<T>\n";
}

// Check partial generation, where parameters start, shape and stride are
// actually checked
template<typename T, std::size_t NDIM>
void validate_part(std::array<Index, NDIM> underlying_shape,
        std::array<Index, NDIM> start, std::array<Index, NDIM> shape)
{
    // Set default values for tests
    T mean = 0, stddev = 1;
    unsigned long long seed = CORE_rnd64_jump(1000, -1);
    // Init strides
    std::array<Index, NDIM> stride, tmp_index;
    stride[0] = 2;
    Index underlying_nelems = underlying_shape[0];
    Index nelems = shape[0];
    Index size = (shape[0]-1)*stride[0] + 1;
    for(Index i = 1; i < NDIM; ++i)
    {
        stride[i] = stride[i-1]*shape[i-1] + 1; // Stride is larger than needed
        underlying_nelems *= underlying_shape[i];
        nelems *= shape[i];
        size += (shape[i]-1) * stride[i];
    }
    // Init reference array
    std::vector<T> underlying_array(underlying_nelems);
    unsigned long long seed2 = seed;
    for(Index i = 0; i < underlying_nelems; ++i)
    {
        underlying_array[i] = chameleon_randn(seed2, mean, stddev);
    }
    // Run kernel
    std::vector<T> data(size);
    std::cout << "Run kernel::randn::cpu<T>\n";
    cpu(NDIM, nelems, seed, mean, stddev, &start[0], &shape[0],
            &underlying_shape[0], &data[0], &stride[0], &tmp_index[0]);
    // Check if the result is the same as the reference one
    for(Index i = 0; i < nelems; ++i)
    {
        // Get index of the current element within output array
        Index offset = i;
        std::array<Index, NDIM> index;
        for(Index j = 0; j < NDIM; ++j)
        {
            index[j] = offset % shape[j];
            offset /= shape[j];
        }
        // Cast it into index within underlying array
        std::array<Index, NDIM> underlying_index;
        for(Index j = 0; j < NDIM; ++j)
        {
            underlying_index[j] = index[j] + start[j];
        }
        // Convert underlying index to underlying memory offset
        Index underlying_offset = underlying_index[NDIM-1];
        for(Index j = NDIM-2; j >= 0; --j)
        {
            underlying_offset = underlying_index[j]
                + underlying_offset*underlying_shape[j];
        }
        // Convert index to memory offset
        offset = 0;
        for(Index j = 0; j < NDIM; ++j)
        {
            offset += stride[j] * index[j];
        }
        // Compare results
        TEST_ASSERT(data[offset] == underlying_array[underlying_offset]);
    }
    std::cout << "OK: kernel::randn::cpu<T>\n";
}

// Run multiple tests for a given precision
template<typename T>
void validate_many()
{
    validate_empty_shape<T>();
    validate_full<T, 1>({1});
    validate_full<T, 2>({2, 3});
    validate_full<T, 4>({3, 4, 5, 6});
    validate_full<T, 2>({1000, 1000});
    validate_part<T, 1>({1}, {0}, {1});
    validate_part<T, 2>({2, 3}, {0, 0}, {1, 1});
    validate_part<T, 2>({2, 3}, {0, 0}, {1, 1});
    validate_part<T, 2>({2, 3}, {1, 2}, {1, 1});
    validate_part<T, 4>({3, 4, 5, 6}, {0, 0, 0, 0}, {2, 4, 2, 3});
    validate_part<T, 4>({3, 4, 5, 6}, {1, 2, 1, 3}, {2, 2, 3, 3});
    validate_part<T, 2>({1000, 1000}, {450, 450}, {450, 450});
}

int main(int argc, char **argv)
{
    validate_many<fp32_t>();
    validate_many<fp64_t>();
    return 0;
}
