/*
kernels.cu

Description:
  A CUDA source code file for simple buffer setting and validation operations.

Author: Nathan Hanford

Copyright (c) 2026 Lawrence Livermore National Security (LLNS), LLC.
See COPYRIGHT file for more details.

SPDX-License-Identifier: MIT
See LICENSE file for more details
*/

#include <cuda_runtime.h>

// Pure CUDA/HIP kernels - no MPI dependency
__global__ void init_buffer_kernel(char* buffer, size_t size, int rank)
{
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < size) {
        buffer[i] = (char) ((i+1)*(rank+1) + i);
    }
}

__global__ void check_buffer_kernel(char* buffer, size_t size, int rank, int* error_flag, size_t* error_index)
{
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < size) {
        char expected = (char) ((i+1)*(rank+1) + i);
        if (buffer[i] != expected) {
            atomicExch(error_flag, 1);
            atomicMin((unsigned long long*)error_index, (unsigned long long)i);
        }
    }
}

__global__ void check_rbuffer_kernel(char* buffer, size_t byte_offset, int rank,
                                      size_t src_byte_offset, size_t element_count,
                                      int* error_flag, size_t* error_index)
{
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < element_count) {
        size_t j = src_byte_offset + i;
        char expected = (char) ((j+1)*(rank+1) + j);
        if (buffer[byte_offset + i] != expected) {
            atomicExch(error_flag, 1);
            atomicMin((unsigned long long*)error_index, (unsigned long long)(byte_offset + i));
        }
    }
}

// C-callable wrapper functions
extern "C" {

void launch_init_buffer_kernel(char* buffer, size_t size, int rank, int numBlocks, int blockSize)
{
    init_buffer_kernel<<<numBlocks, blockSize>>>(buffer, size, rank);
    cudaDeviceSynchronize();
}

void launch_check_buffer_kernel(char* buffer, size_t size, int rank,
                                 int* error_flag, size_t* error_index,
                                 int numBlocks, int blockSize)
{
    check_buffer_kernel<<<numBlocks, blockSize>>>(buffer, size, rank, error_flag, error_index);
    cudaDeviceSynchronize();
}

void launch_check_rbuffer_kernel(char* buffer, size_t byte_offset, int rank,
                                  size_t src_byte_offset, size_t element_count,
                                  int* error_flag, size_t* error_index,
                                  int numBlocks, int blockSize)
{
    check_rbuffer_kernel<<<numBlocks, blockSize>>>(buffer, byte_offset, rank,
                                                    src_byte_offset, element_count,
                                                    error_flag, error_index);
    cudaDeviceSynchronize();
}

} // extern "C"
