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

download_zlib()
{
    download_package 'zlib-1.3.1' \
        --sig '%.asc' --key '5ED46A6721D365587791E2AA783FCD8E58BCAFBA' \
        "https://www.zlib.net/zlib-1.3.1.tar.gz"
}

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[zlib]}" "$src_dir"
}
export -f do_extract

filter_out_args()
{
    local -a patterns=()
    local -a args=()
    local ok
    while (( $# )); do
        [ "$1" != '--' ] || { shift; break; }
        patterns+=("$1")
        shift
    done
    while (( $# )); do
        ok='y'
        for pattern in "${patterns[@]}"; do
            [[ "$1" != $pattern ]] || { ok=''; break; }
        done
        [ ! "$ok" ] || args+=("$1")
        shift
    done
    "${args[@]}"
}
export -f filter_out_args

do_configure()
{
    set -exo pipefail
    local -a flags=("${configure_flags[@]}")
    cd "$build_dir"
    # because zlib is weird and doesn't use --host/--build, we have to look for
    # Darwin and add in an arch flag to make it behave properly
    case "${host_platform}" in
    Darwin)
        flags+=(-archs='-arch x86_64')
        ;;
    esac
    # filter out unsupported args to zlib's overly-simplistic configure
    filter_out_args '--host=*' '--build=*' -- \
        "$src_dir/configure" "${flags[@]}" --static --prefix="$prefix"
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
    download_zlib >/dev/null \
        || fatal "failed to download zlib"
    exit 0
fi

################################################################################

export -A files=()

if ! is_source_extracted; then
    files[zlib]="$(download_zlib)" \
        || fatal "failed to download zlib"
    log_command 'extracting' -- do_extract
    mark_source_extracted
fi
if needs_configured; then
    log_command 'configuring' -- do_configure
fi
log_command 'building' -- do_build
log_command 'installing' -- do_install
finalize_package

