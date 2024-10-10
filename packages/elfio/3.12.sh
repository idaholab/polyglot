#!/this/should/be/included/bash

################################################################################
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


GH_RELEASES_URL='https://github.com/serge1/ELFIO/releases/download'

download_elfio()
{
    download_package 'elfio-3.12' \
        "${GH_RELEASES_URL}/Release_3.12/elfio-3.12.tar.gz"
}

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[elfio]}" "$src_dir"
}
export -f do_extract

do_configure()
{
    set -exo pipefail
    cd "$build_dir"
    cmake "$src_dir" \
        "${cmake_flags[@]}" \
        -DCMAKE_INSTALL_PREFIX="$prefix"
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
    download_elfio >/dev/null \
        || fatal "failed to download elfio"
    exit 0
fi

################################################################################

export -A files=()

if ! is_source_extracted; then
    files[elfio]="$(download_elfio)" \
        || fatal "failed to download elfio"
    log_command 'extracting' -- do_extract
    mark_source_extracted
fi
if needs_configured; then
    log_command 'configuring' -- do_configure
fi
log_command 'building' -- do_build
log_command 'installing' -- do_install
finalize_package

