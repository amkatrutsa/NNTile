# @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
#                          (Skoltech). All rights reserved.
#
# NNTile is software framework for fast training of big neural networks on
# distributed-memory heterogeneous systems based on StarPU runtime system.
#
# @file tests/starpu/CMakeLists.txt
# Tests for nntile::starpu functions
#
# @version 1.0.0
# @author Aleksandr Mikhalev
# @date 2022-08-10

# All unit tests without arguments to test executable
set(TESTS
    "bias"
    "clear"
    "copy"
    "gelu"
    "gelutanh"
    "normalize"
    "randn"
    "sumnorm"
    )

foreach(test IN LISTS TESTS)
    add_test_set(TARGET_NAME tests_starpu_${test}
        EXEC_NAME test_${test}
        SOURCES ${test}.cc
        LINK_LIBRARIES nntile
        COV_ENABLE ${BUILD_COVERAGE}
        COV_NAME coverage_starpu_${test}
        COV_GLOBAL coverage
        )
endforeach()

# Explicitly add gemm test since it waas generated from gemm.cc.in
add_test_set(TARGET_NAME tests_starpu_gemm
    EXEC_NAME test_gemm
    SOURCES "${CMAKE_CURRENT_BINARY_DIR}/gemm.cc"
    LINK_LIBRARIES nntile BLAS::BLAS
    COV_ENABLE ${BUILD_COVERAGE}
    COV_NAME coverage_starpu_gemm
    COV_GLOBAL coverage
    )
