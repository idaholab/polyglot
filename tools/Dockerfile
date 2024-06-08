####################################################################################################
## Dockerfile to setup a QEMU Linux userspace testing environment
##--------------------------------------------------------------------------------------------------
## This is of primary use under macOS/*NIX/etc. (since QEMU can't provide Linux userspace targets on
## non-Linux systems), but may be useful on a variety of hosts as a convenient way to provide QEMU
## Linux userspace binaries if no standard package exists (or for cleanliness).
####################################################################################################

## BASIC ENVIRONMENT SETUP #########################################################################

# we really don't care what Debian version we're based on at this point
FROM debian
# make some tweaks to our environment as the very first step
RUN <<EOF1
cat >>~/.bashrc <<EOF2
set -o vi
EOF2
EOF1
# our "normal" command is to just run bash; this could change in the future if we had a more proper
# "test environment" that used some special entrypoint
CMD /bin/bash
# update apt packages
RUN apt update
# install the standard QEMU Linux userspace binaries (note that we need the statically linked
# versions; the non-static ones explode in some cases, especially in ARM64 environments, which is
# bad for testing under macOS on Apple silicon...)
RUN apt install -y  qemu-user-static
# install Python (technically this is a build-dep for QEMU too, but it's also just Python...)
RUN apt install -y  python3 python3-pip

## QEMU BUILD PREP #################################################################################

# install build dependencies for QEMU, since we need some extra targets that modern QEMU has removed
# support for (namely Tile-GX)
RUN apt install -y  build-essential file libglib2.0-dev ninja-build pkg-config wget

## BUILD QEMU-5.2.0 WITH ADDITIONAL DEPRECATED TARGETS #############################################

RUN <<EOF
# build in a standalone directory in /tmp
mkdir -p /tmp/qemu-5.2.0-build; cd /tmp/qemu-5.2.0-build
# download and extract our source archive (NOTE: we could probably do this without having to install
# wget and instead acquiring it on the outside and just using an ADD to pull the file into our
# container, but this is simpler for right now)
wget https://download.qemu.org/qemu-5.2.0.tar.xz
tar xf qemu-5.2.0.tar.xz
# create a build directory and move into it, then configure/build/install
mkdir build; cd build
../qemu-5.2.0/configure \
    --static                        `# we want statically linked binaries `\
    --disable-system                `# we dont care about full-system emulation `\
    --prefix=/opt/qemu-5.2.0        `# put it out in /opt for cleanliness/sanity `\
    --target-list=tilegx-linux-user `# specify our specific targets `
make
make install
# for consistency, symlink all our userspace binaries to '*-static' analogues (makes running them
# from outside the container easier, since this is how the qemu-user-static package does it)
for exe in $(ls /opt/qemu-5.2.0/bin/qemu-*); do
    ln -s ${exe} /opt/qemu-5.2.0/bin/${exe}-static
done
# cleanup our build directory
cd; rm -rf /tmp/qemu-5.2.0-build
EOF
# add our bonus QEMU target binaries to the search path
ENV PATH="/opt/qemu-5.2.0/bin:${PATH}"

