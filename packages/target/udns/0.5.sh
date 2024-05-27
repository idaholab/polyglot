#!/this/should/be/included/bash

# This file is part of Polyglot.
#
# Copyright (C) 2024, Battelle Energy Alliance, LLC ALL RIGHTS RESERVED
#
# Polyglot is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3, or (at your option) any later version.
#
# Polyglot is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this software; if not see <http://www.gnu.org/licenses/>.

################################################################################

export target="${options[target]}"

################################################################################

download_udns()
{
    set -eo pipefail
    local local_archive="$(find_package_files 'src/udns-0.5.tar.gz')"
    download_package 'udns-0.5' \
        --checksum 'c9a6982fb5f5a5f223a431ad5932d916aa78a0db' \
        "${local_archive:+file://$local_archive}" \
        "https://www.corpit.ru/mjt/udns/udns-0.5.tar.gz"
}

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[udns]}" "$src_dir"
}
export -f do_extract

do_patch()
{
    set -exo pipefail
    cd "$src_dir"
    batch_apply_patches $(find_package_files "patches/*.patch")
}
export -f do_patch

do_configure()
{
    set -exo pipefail
    rsync -Ldru "$src_dir/" "$build_dir/"
    cd "$build_dir"
    CC="$target-gcc" \
    AR="$target-ar" \
    "./configure" \
        --disable-ipv6
}
export -f do_configure

do_build()
{
    set -exo pipefail
    make -C "$build_dir" "${make_flags[@]}" staticlib
}
export -f do_build

do_install_headers()
{
    set -exo pipefail
    mkdir -p "$target_prefix/include"
    cp -f "$src_dir/udns.h" "$target_prefix/include/"
}
export -f do_install_headers

do_install()
{
    set -exo pipefail
    cp -f "$build_dir/libudns.a" "$target_prefix/lib/"
}
export -f do_install

################################################################################

set_target_package

export -A files=()
if ! is_source_extracted; then
    files[udns]="$(download_udns)" || fatal "failed to download udns"
    log_command 'extracting' -- do_extract
    log_command 'patching' -- do_patch
    mark_source_extracted
fi

case "${config:-default}" in
install-headers)
    log_command 'installing headers' -- do_install_headers
    finalize_package --keep-src
    ;;
default)
    if needs_configured; then
        log_command 'configuring' -- do_configure
    fi
    log_command 'building' -- do_build
    log_command 'installing' -- do_install
    finalize_package --keep-src
    ;;
*)
    fatal "unknown configuration: $config"
    ;;
esac


