/*! @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file tests/tile/copy_intersection.cc
 * Copy intersection of 2 tiles from one into another
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2022-09-12
 * */

#include "nntile/tile/copy_intersection.hh"
#include "nntile/starpu/subcopy.hh"
#include "../testing.hh"
#include "../starpu/common.hh"

using namespace nntile;
using namespace nntile::tile;

template<typename T>
void validate()
{
    Tile<T> tile1({}), tile2({2, 2, 3}), tile3({2, 3, 4});
    // Check full copying, that is delegated to starpu_data_cpy internally
    auto tile1_local = tile1.acquire(STARPU_W);
    tile1_local[0] = T{-1};
    tile1_local.release();
    auto tile2_local = tile2.acquire(STARPU_W);
    for(Index i = 0; i < tile2.nelems; ++i)
    {
        tile2_local[i] = T(i+1);
    }
    tile2_local.release();
    auto tile3_local = tile3.acquire(STARPU_W);
    for(Index i = 0; i < tile3.nelems; ++i)
    {
        tile3_local[i] = T(2*i+2);
    }
    tile3_local.release();
    Tile<T> tile1_copy({});
    copy_intersection<T>(tile1, {}, tile1_copy, {});
    auto tile1_copy_local = tile1_copy.acquire(STARPU_R);
    TEST_ASSERT(tile1_copy_local[0] == T{-1});
    tile1_copy_local.release();
    Tile<T> tile2_copy(tile2.shape);
    copy_intersection<T>(tile2, {0, 0, 0}, tile2_copy, {0, 0, 0});
    auto tile2_copy_local = tile2_copy.acquire(STARPU_RW);
    for(Index i = 0; i < tile2.nelems; ++i)
    {
        TEST_ASSERT(tile2_copy_local[i] == T(i+1));
        tile2_copy_local[i] = T{-2};
    }
    tile2_copy_local.release();
    copy_intersection<T>(tile2, {1, 2, 3}, tile2_copy, {1, 2, 3});
    tile2_copy_local.acquire(STARPU_R);
    for(Index i = 0; i < tile2.nelems; ++i)
    {
        TEST_ASSERT(tile2_copy_local[i] == T(i+1));
    }
    tile2_copy_local.release();
    // Check complex copying on CPU, no CUDA implementation as of now
    StarpuVariableHandle scratch(2*3*sizeof(Index), STARPU_R);
    starpu::subcopy::submit<T>(3, {0, 0, 2}, tile3.stride, {0, 1, 0},
            tile2.stride, {2, 1, 2}, tile3, tile2, scratch, STARPU_RW);
    copy_intersection<T>(tile3, {0, 1, 0}, tile2_copy, {0, 0, 2});
    tile2_local.acquire(STARPU_R);
    tile2_copy_local.acquire(STARPU_R);
    for(Index i = 0; i < tile2.nelems; ++i)
    {
        TEST_ASSERT(tile2_local[i] == tile2_copy_local[i]);
    }
    tile2_local.release();
    tile2_copy_local.release();
    // Checking throwing exceptions
    TEST_THROW(copy_intersection<T>(Tile<T>({1}), {}, Tile<T>({1}), {0}));
    TEST_THROW(copy_intersection<T>(Tile<T>({1}), {0}, Tile<T>({}), {0}));
    TEST_THROW(copy_intersection<T>(Tile<T>({1}), {0}, Tile<T>({1}), {}));
}

int main(int argc, char **argv)
{
    // Init StarPU for testing
    StarpuTest starpu;
    // Init codelet
    starpu::subcopy::init();
    // Launch all tests
    validate<fp32_t>();
    validate<fp64_t>();
    return 0;
}
