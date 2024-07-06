FROM alpine:latest

ARG WORK_DIR=/app
WORKDIR ${WORK_DIR}

RUN apk add --no-cache openrc

COPY --chmod=755 sserver/bin/* /usr/sbin
COPY --chmod=755 sserver/sserver-app /etc/init.d

# Setup
RUN ARCH=`uname -m` && \
    if [ "$ARCH" == "aarch64" ]; then \
      apk add --no-cache libc6-compat; \
      rm -rf /usr/sbin/sserver; \
      mv /usr/sbin/sserver-arm /usr/sbin/sserver; \
    else \
      rm -rf /usr/sbin/sserver-arm; \
    fi; \
    sed -i 's/^\#rc_env_allow=\"VAR1 VAR2\"$/rc_env_allow="\*"/' /etc/rc.conf;
# Setup

# Minidlna
RUN sed -i 's/^(#)(.*)(\/community)$/\2\3/' /etc/apk/repositories; \
    apk add --no-cache minidlna; \
    mkdir /var/cache/minidlna; \
    chown -R minidlna:minidlna /var/cache/minidlna; \
    sed -i 's/#network_interface=eth0/network_interface=eth0,end0,en0,wlan0/;s/media_dir=\/opt/media_dir=V,\/media\/media\/Videos/; s/#friendly_name=My DLNA Server/friendly_name=nAlpine/; s/#db_dir=\/var\/cache\/minidlna/db_dir=\/var\/cache\/minidlna/; s/#log_dir=\/var\/log/log_dir=\/var\/log\/minidlna/' /etc/minidlna.conf; \
    sed -i '/command_args="$command_args -R"/a\\tfi\n\tif yesno "${_RESCAN}"; then\n\t\tcommand_args="$command_args -r"' /etc/init.d/minidlna; \
    sed -i '/RESCAN="false"/a_RESCAN="false"' /etc/conf.d/minidlna; \
    rc-update add minidlna default;
# Minidlna

# Simple Server
RUN rc-update add sserver-app default;
# Simple Server

# Clean Cache
RUN rm -rf /var/cache/apk/*;
# Clean Cache

CMD ["/sbin/init"]
