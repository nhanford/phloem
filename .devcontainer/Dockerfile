FROM fluxrm/flux-core:el10

USER root

RUN dnf clean all \
	&& rm -rf /var/cache/dnf \
	&& git config --global checkout.workers 1024 \
	&& git config --global feature.manyFiles true \
	&& echo "* soft memlock unlimited" >> /etc/security/limits.conf \
    && echo "* hard memlock unlimited" >> /etc/security/limits.conf

USER fluxuser

RUN git clone https://github.com/spack/spack.git \
	&& ./spack/bin/spack repo update --branch=develop builtin \
	&& echo 'source ~/spack/share/spack/setup-env.sh' >> ~/.bashrc
