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
declare -a target_bits=(${target//-/ })
export target_arch="${options[target-arch]:-${target_bits[0]:-unknown}}"
export linux_arch="${options[linux-arch]:-$target_arch}"

################################################################################

check_for_functions download_linux

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[linux]}" "$src_dir"
}
export -f do_extract

do_headers_install()
{
    set -exo pipefail
    make -C "$src_dir" "${make_flags[@]}" \
        headers_install \
        ARCH=$linux_arch \
        INSTALL_HDR_PATH="$build_dir"
    rsync --include="*" -rdu \
        "$build_dir/include/" \
        "$prefix/include/linux-$linux_arch"
}
export -f do_headers_install

################################################################################

export -A files=()
if ! is_source_extracted; then
    files[linux]="$(download_linux)"
    log_command 'extracting' -- do_extract
    mark_source_extracted
fi
log_command 'installing headers' -- do_headers_install
finalize_package

