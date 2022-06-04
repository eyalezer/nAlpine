#!/bin/sh

# install command:
#$ mkdir /tmp/m; mount -t vboxsf Media /tmp/m/; cd /tmp/m/Alpine; ./install.sh; cd /tmp; umount /tmp/m

echo "Installing minidlna, simple server & configuring system..."

cd /tmp
wget -O sserver https://github.com/eyalezer/nAlpine/blob/main/sserver/bin/sserver?raw=true
wget -O sserver-app https://github.com/eyalezer/nAlpine/blob/main/sserver/sserver-app?raw=true

# SSH
echo "Enables ssh for root user"
sed -i 's/#PermitRootLogin prohibit-password|PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
# SSH

# Disk Mount
echo "Enables auto disk mount"
grep -qF 'vboxsf' /etc/fstab || echo "Media		/media/media	vboxsf	uid=0,gid=102	0 0" >> /etc/fstab
# Disk Mount

# Minidlna
echo "Installing Minidlna"
sed -i 's/#http:\/\/dl-cdn.alpinelinux.org\/alpine\/edge\/community/http:\/\/dl-cdn.alpinelinux.org\/alpine\/edge\/community/' /etc/apk/repositories
apk add --no-cache minidlna
mkdir /var/cache/minidlna
chown -R minidlna:minidlna /var/cache/minidlna
sed -i 's/#network_interface=eth0/network_interface=eth1/; s/media_dir=\/opt/media_dir=V,\/media\/media\/Videos/; s/#friendly_name=My DLNA Server/friendly_name=nAlpineDLNA/; s/#db_dir=\/var\/cache\/minidlna/db_dir=\/var\/cache\/minidlna/; s/#log_dir=\/var\/log/log_dir=\/var\/log\/minidlna/' /etc/minidlna.conf
sed -i '/command_args="$command_args -R"/a\\tfi\n\tif yesno "${_RESCAN}"; then\n\t\tcommand_args="$command_args -r"' /etc/init.d/minidlna
sed -i '/RESCAN="false"/a_RESCAN="false"' /etc/conf.d/minidlna
rc-update add minidlna default
# Minidlna

# Simple Server
echo "Installing Simple Server"
cp sserver /usr/sbin/
chmod a+x /usr/sbin/sserver
cp sserver-app /etc/init.d/
chmod a+x /etc/init.d/sserver-app
rc-update add sserver-app default
# Simple Server

# Cleaning up
rm -rf sserver
rm -rf sserver-app

echo "Done - Please Restart"