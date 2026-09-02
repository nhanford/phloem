# Copyright (c) 2017-2026, Lawrence Livermore National Security, LLC.
# See the top-level LICENSE file for details
#
# SPDX-License-Identifier: (MIT)

#------------------------------------------------------------------------------
# HPE Cray cce@20.0.0 compilers
#------------------------------------------------------------------------------
set(CCE_HOME "/usr/tce/packages/cce-tce/cce-20.0.0")
set(CMAKE_C_COMPILER "/opt/rocm-6.4.3/llvm/bin/amdclang" CACHE PATH "")
set(CMAKE_CXX_COMPILER "/opt/rocm-6.4.3/llvm/bin/amdclang++" CACHE PATH "")

# Fortran support
set(ENABLE_FORTRAN OFF CACHE BOOL "")

#------------------------------------------------------------------------------
# MPI Support
#------------------------------------------------------------------------------
set(ENABLE_MPI ON CACHE BOOL "")

set(MPI_HOME "/opt/cray/pe/mpich/9.0.1/ofi/crayclang/20.0")
set(MPI_C_COMPILER "${MPI_HOME}/bin/mpicc" CACHE PATH "")
set(MPI_CXX_COMPILER "${MPI_HOME}/bin/mpicxx" CACHE PATH "")
set(MPI_Fortran_COMPILER "${MPI_HOME}/bin/mpif90" CACHE PATH "")
set(MPIEXEC_EXECUTABLE "/usr/global/tools/flux_wrappers/bin/srun" CACHE PATH "")
set(MPIEXEC_NUMPROC_FLAG "-n" CACHE STRING "")

#------------------------------------------------------------------------------
# HIP support
#------------------------------------------------------------------------------
set(ENABLE_HIP ON CACHE BOOL "")

set(CMAKE_HIP_COMPILER "${CMAKE_CXX_COMPILER}" CACHE FILEPATH "")
set(ROCM_PATH "/opt/rocm-6.4.3/" CACHE PATH "")
set(CMAKE_HIP_ARCHITECTURES "gfx942" CACHE STRING "gfx architecture to use when generating HIP/ROCm code")

# Recommended link line when not using tce-wrapped compilers

set(CMAKE_EXE_LINKER_FLAGS "-lxpmem -L/opt/cray/pe/mpich/9.0.1/gtl/lib -Wl,-rpath,/opt/cray/pe/mpich/9.0.1/gtl/lib -lmpi_gtl_hsa -L/opt/rocm-6.4.3/lib/llvm/lib -Wl,-rpath,/opt/rocm-6.4.3/lib/llvm/lib -L/opt/rocm-6.4.3/lib -Wl,-rpath,/opt/rocm-6.4.3/lib -lpgmath -Wl,--disable-new-dtags -lflang -lflangrti -lamdhip64 -lhsakmt -lhsa-runtime64 -lamd_comgr " CACHE STRING "")
