# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack_repo.builtin.build_systems.cached_cmake import (
    CachedCMakePackage,
    cmake_cache_option,
    cmake_cache_path,
)
from spack_repo.builtin.build_systems.cuda import CudaPackage
from spack_repo.builtin.build_systems.rocm import ROCmPackage
from spack_repo.builtin.packages.blt.package import llnl_link_helpers
from spack.package import *

class Phloem(CachedCMakePackage, CudaPackage, ROCmPackage):
    """A suite of MPI benchmarks with Umpire memory allocation."""

    homepage = "https://lc.llnl.gov/gitlab/phloem/phloem2"
    git = "https://lc.llnl.gov/gitlab/phloem/phloem2.git"

    maintainers("nhanford")

    license("MIT", checked_by="nhanford")

    version("main", branch="main", submodules=False)

    depends_on("blt", type="build")
    depends_on("umpire")
    depends_on("mpi")

    with when("+rocm"):
        for arch_ in ROCmPackage.amdgpu_targets:
            depends_on(f"umpire+rocm amdgpu_target={arch_}", when=f"amdgpu_target={arch_}")
        for impl_ in ["openmpi@5:", "mpich", "mvapich-plus", "cray-mpich"]:
            depends_on(f"{impl_} +rocm", when=f"^[virtuals=mpi] {impl_}")

    with when("+cuda"):
        depends_on("umpire+cuda", when="+cuda")
        for impl_ in ["openmpi@5:", "mpich", "mvapich-plus", "cray-mpich"]:
            depends_on(f"{impl_} +cuda", when=f"^[virtuals=mpi] {impl_}")

    depends_on("c", type="build")
    depends_on("cxx", type="build")


    def initconfig_mpi_entries(self):
        spec = self.spec
        entries = super().initconfig_mpi_entries()
        entries.append(cmake_cache_option("ENABLE_MPI", True))
        entries.append(cmake_cache_path("MPI_HOME", self.spec["mpi"].prefix))
        return entries

    def cmake_args(self):
        args = super().cmake_args()

        args.extend([
            self.define_from_variant("ENABLE_HIP", "rocm"),
            self.define_from_variant("ENABLE_CUDA", "cuda"),
            self.define("BLT_SOURCE_DIR", self.spec["blt"].prefix),
        ])
        if "+rocm" in self.spec and self.spec.satisfies("^cray-mpich"):
            # Append to existing MPI link flags. does not appear influential...
            mpi_link_flags = self.spec["mpi"].libs.link_flags
            mpi_link_flags += "-lmpi_gtl_hsa"
            self.define("BLT_MPI_LINK_FLAGS", mpi_link_flags)
        return args
