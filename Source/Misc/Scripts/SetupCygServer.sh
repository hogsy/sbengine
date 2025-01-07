#!/bin/sh
mkdir '/cygdrive/c/usr/local/cell/Scripts' -p
cp '\\jesper/Scripts' '/cygdrive/c/usr/local/cell/' -a
cp '/cygdrive/c/usr/local/cell/Scripts/rsyncd.conf' /etc/ -a
/usr/bin/cygserver-config -y
mkdir /cygdrive/c/temp
chmod 777 /cygdrive/c/temp
mkdir /cygdrive/c/ccache
chmod 777 /cygdrive/c/ccache
cygrunsrv -S cygserver
cygrunsrv -R distccd
cygrunsrv -I distccd --path "/bin/distccd" --args "--daemon --allow 172.0.0.0/0  --no-detach" -d "CYGWIN distccd" -e "CYGWIN=server" --shutdown -n -e TMPDIR="c:/temp" -e CCACHE_DIR="c:/ccache"
cygrunsrv -S distccd
ssh-host-config -y -c ntsec
cygrunsrv -S sshd
cygrunsrv -R rsyncd
cygrunsrv -I rsyncd --path "/bin/rsync" --args "--daemon"
cygrunsrv -S rsyncd
cygrunsrv -L -V
#mkdir '/cygdrive/c/usr/local/cell/0_8_4/' -p
#cd '/cygdrive/c/usr/local/cell/0_8_4/host-win32/'
#cp '\\jesper/usr/local/cell/0_8_4/host-win32/ppu' . -r
#chmod '/cygdrive/c/usr/local/cell/0_8_4/host-win32/ppu' -R
