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

GH_RELEASES_URL="https://github.com/besser82/libxcrypt/releases/download"

download_libxcrypt()
{
    set -eo pipefail
    download_package 'libxcrypt-4.4.36' \
        --key "678CE3FEE430311596DB8C16F52E98007594C21D" \
        --signature '%.asc' \
        "$GH_RELEASES_URL/v4.4.36/libxcrypt-4.4.36.tar.xz"
}

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[libxcrypt]}" "$src_dir"
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
    cd "$build_dir"
    "$src_dir/configure" \
        --host=$target-local \
        --prefix="$target_prefix" \
        --without-pic \
        --disable-werror \
        --disable-shared \
        --enable-hashes=all
}
export -f do_configure

do_build()
{
    set -exo pipefail
    make -C "$build_dir" "${make_flags[@]}"
}
export -f do_build

do_install()
{
    set -exo pipefail
    make -C "$build_dir" "${make_install_flags[@]}" install
}
export -f do_install

################################################################################

if [[ ${options[only-cache]:-0} == 1 ]]; then
    download_libxcrypt >/dev/null \
        || fatal "failed to download libxcrypt"
    exit 0
fi

################################################################################

set_target_package

export -A files=()
if ! is_source_extracted; then
    files[libxcrypt]="$(download_libxcrypt)" \
        || fatal "failed to download libxcrypt"
    log_command 'extracting' -- do_extract
    log_command 'patching' -- do_patch
    mark_source_extracted
fi
if needs_configured; then
    log_command 'configuring' -- do_configure
fi
log_command 'building' -- do_build
log_command 'installing' -- do_install
finalize_package --keep-src

