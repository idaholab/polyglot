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

download_zstd()
{
    download_package 'zstd-1.5.6' \
        --sig '%.sig' --key '4EF4AC63455FC9F4545D9B7DEF8FE99528B52FFD' \
        "https://github.com/facebook/zstd/releases/download/v1.5.6/zstd-1.5.6.tar.gz"
}

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[zstd]}" "$src_dir"
}
export -f do_extract

do_build()
{
    set -exo pipefail
    cp -rf "$src_dir"/* "$build_dir/"
    # clean first, since zstd can end up in a weird incomplete build state
    make -C "$build_dir" "${make_flags[@]}" clean
    # then build everything
    make -C "$build_dir" "${make_flags[@]}"
}
export -f do_build

do_install()
{
    set -exo pipefail
    make -C "$build_dir/lib" "${make_install_flags[@]}" \
        install-pc install-static install-includes \
        prefix="$prefix"
}
export -f do_install

################################################################################

export -A files=()

if ! is_source_extracted; then
    files[zstd]="$(download_zstd)"
    log_command 'extracting' -- do_extract
    mark_source_extracted
fi
log_command 'building' -- do_build
log_command 'installing' -- do_install
finalize_package

