#!/bin/bash
set -e

mkdir -p /home/fluxuser/spack/var/spack/environments/phloem/
ln -sf /workspaces/phloem2/.devcontainer/spack.yaml /home/fluxuser/spack/var/spack/environments/phloem/spack.yaml

#cmake -B build -C .devcontainer/host-config.cmake -DDOCS=ON
