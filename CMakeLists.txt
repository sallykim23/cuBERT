cmake_minimum_required(VERSION 3.6)
project(cuBERT VERSION 0.0.5 LANGUAGES C CXX)

#ADDED mpi
SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpicxx)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

option(cuBERT_ENABLE_GPU "Enable GPU support" OFF)
option(cuBERT_ENABLE_MKL_SUPPORT "Enable Intel MKL support" ON)
option(cuBERT_SYSTEM_MKL "Use system MKL" OFF)
option(cuBERT_SYSTEM_PROTOBUF "Use system Protobuf" OFF)

find_package(Threads REQUIRED)

# OpenMP Support
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    #saving the openmpi path for the compilation of the mpi_added_executables
    include_directories(${OPENMPI_PATH})
endif()

include(cub)
include(utf8proc)
include(protobuf-c)
include(googletest )

set(cuBERT_EXTERNAL_DEPENDENCIES
        cub
        utf8proc
        protobuf-c
        googletest)

set(cuBERT_EXTERNAL_LIBRARIES
        ${protobuf-c_STATIC_LIBRARIES}
        ${utf8proc_STATIC_LIBRARIES})

include_directories(
        # Source and generated code.
        src
        ${CMAKE_CURRENT_BINARY_DIR}
        # External dependencies.
        ${cub_INCLUDE_DIR}
        ${protobuf-c_INCLUDE_DIRS}
        ${utf8proc_INCLUDE_DIRS})

# MKL Support
if(cuBERT_ENABLE_MKL_SUPPORT)
    add_definitions(-DHAVE_MKL)
    if(cuBERT_SYSTEM_MKL)
        find_package(MKL REQUIRED)
    else()
        include(mkl)
        list(APPEND cuBERT_EXTERNAL_DEPENDENCIES mkl_copy_shared_to_destination)
    endif()
    list(APPEND cuBERT_EXTERNAL_LIBRARIES ${MKL_LIBRARIES})
    include_directories(${MKL_INCLUDE_DIR})
endif(cuBERT_ENABLE_MKL_SUPPORT)

if(cuBERT_ENABLE_GPU)
    # minimum 9.0 in cuda version
    find_package(CUDA 9.0 REQUIRED)
    if(NOT CUDA_FOUND)
        message(FATAL_ERROR "CUDA not found.")
    endif()

    add_definitions(-DHAVE_CUDA)

    # use cmake internal CUDA_ARCH_NAME switch
    # e.g. CUDA_ARCH_NAME="Auto" will autodetect
    #      CUDA_ARCH_NAME="All"  will use all arches
    cuda_select_nvcc_arch_flags(NVCC_ARCH_FLAGS ${CUDA_ARCH_NAME})
    list(APPEND CUDA_NVCC_FLAGS ${NVCC_ARCH_FLAGS})
    message(STATUS "Using CUDA arch flags: ${NVCC_ARCH_FLAGS_readable}")

    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-std=c++11)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};--expt-relaxed-constexpr)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-O3;-use_fast_math;-Xptxas -O3;-Xcompiler -O3)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-ftz=true)  # Flush denormals to zero

    include_directories(${CUDA_INCLUDE_DIRS})
endif(cuBERT_ENABLE_GPU)

# Let's get to work!
include(tf_core_framework)

set(SOURCE_FILES
        src/cuBERT.cpp
        src/cuBERT/common.cpp
        src/cuBERT/tokenization.cpp
        src/cuBERT/Bert.cpp
        src/cuBERT/BertM.cpp
        src/cuBERT/tensorflow/Graph.cpp
        src/cuBERT/op/GELU.cpp src/cuBERT/op/GELU.cu
        src/cuBERT/op/Dense.cpp
        src/cuBERT/op/Embedding.cpp src/cuBERT/op/Embedding.cu
        src/cuBERT/op/Softmax.cpp src/cuBERT/op/Softmax.cu
        src/cuBERT/op/LayerNorm.cpp src/cuBERT/op/LayerNorm.cu
        src/cuBERT/op_att/AttentionSelf.cpp
        src/cuBERT/op_att/AttentionMask.cpp src/cuBERT/op_att/AttentionMask.cu
        src/cuBERT/op_att/BatchMatMul.cpp
        src/cuBERT/op_att/Transformer.cpp
        src/cuBERT/op_bert/BertEmbeddings.cpp
        src/cuBERT/op_bert/BertPooler.cpp src/cuBERT/op_bert/BertPooler.cu
        src/cuBERT/op_out/AdditionalOutputLayer.cpp)

if (cuBERT_ENABLE_GPU)
    cuda_add_library(cuBERT SHARED ${SOURCE_FILES})
    set_target_properties(cuBERT PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
    cuda_add_cublas_to_target(cuBERT)
else ()
    add_library(cuBERT SHARED ${SOURCE_FILES})
endif ()
add_dependencies(cuBERT ${cuBERT_EXTERNAL_DEPENDENCIES})
target_link_libraries(cuBERT tf_protos_cc ${cuBERT_EXTERNAL_LIBRARIES})

add_executable(cuBERT_benchmark benchmark/benchmark_cu.cpp)
target_link_libraries(cuBERT_benchmark cuBERT)

find_package(tensorflow)
if (tensorflow_FOUND)
    add_executable(tfBERT_benchmark benchmark/benchmark_tf.cpp)
    target_include_directories(tfBERT_benchmark PUBLIC ${tensorflow_INCLUDE_DIR})
    target_link_libraries(tfBERT_benchmark ${tensorflow_LIBRARIES})
endif ()

if (cuBERT_ENABLE_GPU)
    add_executable(gemm_benchmark benchmark/benchmark_gemm.cpp)
    target_link_libraries(gemm_benchmark cuBERT)
endif ()

enable_testing()
configure_file(test_vocab.txt ${CMAKE_CURRENT_BINARY_DIR}/test_vocab.txt COPYONLY)
add_executable(cuBERT_test test/unit_test.cpp
        test/cuBERT/common_test.cpp
        test/cuBERT/tokenization_test.cpp
        test/cuBERT/op/DenseTest.cpp
        test/cuBERT/op/LayerNormTest.cpp
        test/cuBERT/op/SoftmaxTest.cpp
        test/cuBERT/op/GELUTest.cpp
        test/cuBERT/op/EmbeddingTest.cpp
        test/cuBERT/op_att/BatchMatMulTest.cpp
        test/cuBERT/op_att/AttentionSelfTest.cpp
        test/cuBERT/op_att/TransformerTest.cpp
        test/cuBERT/op_att/AttentionMaskTest.cpp
        test/cuBERT/op_bert/BertEmbeddingsTest.cpp
        test/cuBERT/op_bert/BertPoolerTest.cpp
        test/cuBERT/op_out/AdditionalOutputLayerTest.cpp
        test/cuBERT/BertTest.cpp
        test/cuBERT/BertMTest.cpp
        test/cuBERT_test.cpp)

set(PAPICNT_DIR "${CMAKE_SOURCE_DIR}/../Vapro/VAPRO/build_dummy3")
target_link_libraries(cuBERT_test cuBERT ${googletest_STATIC_LIBRARIES} ${OPENMPI_LIBRARIES} "${PAPICNT_DIR}/libpapicnt.so") 
target_include_directories(cuBERT_test PUBLIC ${googletest_INCLUDE_DIRS} ${OPENMPI_PATH})
add_test(UnitTest cuBERT_test)


install(TARGETS cuBERT
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
install(FILES ${CMAKE_SOURCE_DIR}/src/cuBERT.h DESTINATION include)

if(NOT ${cuBERT_SYSTEM_MKL})
    install(FILES ${MKL_LIBRARIES} DESTINATION lib)
    install(DIRECTORY ${MKL_INCLUDE_DIR} DESTINATION .)
endif()
