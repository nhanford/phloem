/*
kernels.cpp

Description:
  A HIP source code file for simple buffer setting and validation operations.

Author: Nathan Hanford

Copyright (c) 2026 Lawrence Livermore National Security (LLNS), LLC.
See COPYRIGHT file for more details.

SPDX-License-Identifier: MIT
See LICENSE file for more details
*/

#include <hip/hip_runtime.h>

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
    hipLaunchKernelGGL(init_buffer_kernel, dim3(numBlocks), dim3(blockSize), 0, 0,
                       buffer, size, rank);
    hipDeviceSynchronize();
}

void launch_check_buffer_kernel(char* buffer, size_t size, int rank,
                                int* error_flag, size_t* error_index,
                                int numBlocks, int blockSize)
{
    hipLaunchKernelGGL(check_buffer_kernel, dim3(numBlocks), dim3(blockSize), 0, 0,
                       buffer, size, rank, error_flag, error_index);
    hipDeviceSynchronize();
}

void launch_check_rbuffer_kernel(char* buffer, size_t byte_offset, int rank,
                                 size_t src_byte_offset, size_t element_count,
                                 int* error_flag, size_t* error_index,
                                 int numBlocks, int blockSize)
{
    hipLaunchKernelGGL(check_rbuffer_kernel, dim3(numBlocks), dim3(blockSize), 0, 0,
                       buffer, byte_offset, rank, src_byte_offset, element_count,
                       error_flag, error_index);
    hipDeviceSynchronize();
}

} // extern "C"
