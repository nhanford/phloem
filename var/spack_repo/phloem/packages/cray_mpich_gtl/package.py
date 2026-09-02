# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import os
import os.path
import stat

from spack.package import *
from spack.package import LinkTree
from spack_repo.builtin.build_systems.bundle import BundlePackage
from spack_repo.builtin.packages.mpich.package import MpichEnvironmentModifications


class CrayMpichGtl(MpichEnvironmentModifications, BundlePackage):

    version("1.0.0")

    depends_on("cray-mpich", type="build")
    provides("mpi@3")

    @property
    def libs(self):
        libs = self.spec["cray-mpich"].libs
        if self.spec["cray-mpich"].satisfies("+rocm"):
            libs += find_libraries(["libmpi_gtl_hsa"], root=self.prefix.lib, recursive=True)
            libs += find_libraries(["libmpi_gtl_hsa"], root=self.prefix.lib64, recursive=True)
        elif self.spec["cray-mpich"].satisfies("+cuda"):
            libs += find_libraries(["libmpi_gtl_cuda"], root=self.prefix.lib, recursive=True)
            libs += find_libraries(["libmpi_gtl_cuda"], root=self.prefix.lib64, recursive=True)
        return libs

    def setup_dependent_build_environment(self, env, dependent_spec):
        if dependent_spec.satisfies("+rocm"):
            env.set("SPACK_GTL", "hip")
            env.append_flags("-DCMAKE_MPI_C_LINK_FLAGS", "-lmpi_gtl_hsa")
            env.append_flags("BLT_MPI_LINK_FLAGS", "-lmpi_gtl_hsa")
        elif dependent_spec.satisfies("+cuda"):
            env.set("SPACK_GTL", "cuda")
            env.append_flags("-DCMAKE_MPI_C_LINK_FLAGS", "-lmpi_gtl_cuda")
            env.append_flags("BLT_MPI_LINK_FLAGS", "-lmpi_gtl_cuda")

    def setup_dependent_run_environment(self, env, dependent_spec):
        if dependent_spec.satisfies("+rocm") or dependent_spec.satisfies("+cuda"):
            env.set("MPICH_GPU_SUPPORT_ENABLED", "1")

    def setup_run_environment(self, env: EnvironmentModifications) -> None:
        self.setup_mpi_wrapper_variables(env)

    def setup_dependent_package(self, module, dependent_spec):
        MpichEnvironmentModifications.setup_dependent_package(
            self, module, dependent_spec
        )

    def install(self, spec, prefix):
        dep = spec["cray-mpich"]
        for subdir in os.listdir(dep.prefix):
            if subdir == "bin":
                continue
            link_tree = LinkTree(os.path.join(dep.prefix, subdir))
            link_tree.merge(os.path.join(self.prefix, subdir))

        mkdir(self.prefix.bin)
        for target in os.listdir(dep.prefix.bin):
            if target in ["mpicc", "mpicxx", "mpif90", "mpif77"]:
                fpath = os.path.join(self.prefix.bin, target)
                with open(fpath, "w") as f:
                    f.write(
                        f"""\
#!/bin/bash

if [[ "$SPACK_GTL" == "hip" ]]; then
    addlibs="-lmpi_gtl_hsa"
elif [[ "$SPACK_GTL" == "cuda" ]]; then
    addlibs="-lmpi_gtl_cuda"
else
    addlibs=""
fi

{dep.prefix.bin}/{target} "$addlibs" "$@"
"""
                    )
                st = os.stat(fpath)
                os.chmod(fpath, st.st_mode | stat.S_IEXEC)
