#!/bin/bash
set -e

mkdir -p /home/fluxuser/opt/spack/var/spack/environments/phloem/
ln -sf /workspaces/phloem/.devcontainer/spack.yaml /home/fluxuser/opt/spack/var/spack/environments/phloem/spack.yaml
