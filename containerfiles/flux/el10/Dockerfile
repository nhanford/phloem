FROM fluxrm/flux-core:el10

USER root

COPY var/certs/ /tmp/extra-certs/

RUN find /tmp/extra-certs -type f \( -name '*.crt' -o -name '*.pem' \) \
    -exec cp {} /etc/pki/ca-trust/source/anchors/ \; \
    && update-ca-trust \
	&& dnf install -y bzip2 xz \
	&& dnf clean all \
	&& rm -rf /var/cache/dnf \
	&& git config --global checkout.workers 1024 \
	&& git config --global feature.manyFiles true \
    && git config --global core.sharedRepository false \
	&& echo "* soft memlock unlimited" >> /etc/security/limits.conf \
    && echo "* hard memlock unlimited" >> /etc/security/limits.conf

USER fluxuser
WORKDIR ${HOME}

RUN mkdir -p opt \
    && git clone https://github.com/LLNL/mpibind.git \
	&& cd mpibind \
	&& ./bootstrap \
	&& ./configure --prefix=/home/fluxuser/opt/ \
	&& make -j install \
	&& cd .. \
	&& rm -rf mpibind \
	&& cd opt \
	&& git clone https://github.com/spack/spack.git \
	&& ./spack/bin/spack repo update \
	&& ./spack/bin/spack bootstrap now \
	&& echo 'source /home/fluxuser/opt/spack/share/spack/setup-env.sh' >> ~/.bashrc

COPY --chown=fluxuser:fluxuser . /home/fluxuser/phloem

RUN mkdir -p /home/fluxuser/opt/spack/var/spack/environments/phloem/ \
	&& ln -sf /home/fluxuser/phloem/containerfiles/flux/el10/spack.yaml \
	/home/fluxuser/opt/spack/var/spack/environments/phloem/spack.yaml

ENV FLUX_SHELL_RC_PATH=/home/fluxuser/opt/share/mpibind:${FLUX_SHELL_RC_PATH:-}

ENTRYPOINT ["/usr/bin/flux", "start", "/bin/bash"]
