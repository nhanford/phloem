FROM fluxrm/flux-core:el10

USER root

RUN dnf clean all \
	&& rm -rf /var/cache/dnf \
	&& git config --global checkout.workers 1024 \
	&& git config --global feature.manyFiles true \
    && git config --global core.sharedRepository false \
	&& echo "* soft memlock unlimited" >> /etc/security/limits.conf \
    && echo "* hard memlock unlimited" >> /etc/security/limits.conf

USER fluxuser

ENV MPIBIND_INSTALL_PREFIX=/home/fluxuser/mpibind-install

RUN source /etc/profile.d/modules.sh \
    && module load mpi \
    && git clone https://github.com/LLNL/mpibind.git ${GIT_CLONE_STAGING_DIR}/mpibind \
	&& cd ${GIT_CLONE_STAGING_DIR}/mpibind \
	&& ./bootstrap \
	&& ./configure --prefix=${MPIBIND_INSTALL_PREFIX} \
	&& make -j install \
	&& git clone https://github.com/spack/spack.git \
	&& ./spack/bin/spack repo update \
	&& echo 'source ~/spack/share/spack/setup-env.sh' >> ~/.bashrc

COPY --chown=fluxuser:fluxuser . /home/fluxuser/phloem

RUN mkdir -p /home/fluxuser/spack/var/spack/environments/phloem/ \
	&& ln -sf /home/fluxuser/phloem/containerfiles/flux/el10/spack.yaml \
	/home/fluxuser/spack/var/spack/environments/phloem/spack.yaml

ENTRYPOINT ["exec /usr/bin/flux start /bin/bash"]
