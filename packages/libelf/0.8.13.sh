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

download_libelf()
{
    download_package 'libelf-0.8.13' \
        --checksum c1d6ac5f182d19dd685c4dfd74eedbfe3992425d \
        'https://ftp.netbsd.org/pub/pkgsrc/distfiles/libelf-0.8.13.tar.gz'
}

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[libelf]}" "$src_dir"
}
export -f do_extract

do_configure()
{
    set -exo pipefail
    cd "$build_dir"
    CFLAGS="-Wno-implicit-int -Wno-implicit-function-declaration -Wno-error" \
    "$src_dir/configure" \
        "${configure_flags[@]}" \
        --prefix="$prefix" \
        --enable-compat \
        --enable-extended-format
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

export -A files=()

if ! is_source_extracted; then
    files[libelf]="$(download_libelf)" \
        || fatal "failed to download libelf"
    log_command 'extracting' -- do_extract
    mark_source_extracted
fi
if needs_configured; then
    log_command 'configuring' -- do_configure
fi
log_command 'building' -- do_build
log_command 'installing' -- do_install
finalize_package

