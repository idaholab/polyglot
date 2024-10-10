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

lib_make()
{
    set -exo pipefail
    make -f$(locate_file PGT_DATA 'libs/libc/Makefile') \
        $(locate_all_files PGT_DATA 'libs/common' '-I%s') \
        target=$target \
        prefix="$prefix" \
        cachedir="$build_dir" \
        "$@"
}
export -f lib_make

################################################################################

do_install_headers()
{
    set -ex -o pipefail
    lib_make headers-install
}
export -f do_install_headers

do_build()
{
    set -ex -o pipefail
    lib_make all
}
export -f do_build

do_install()
{
    set -ex -o pipefail
    lib_make install
}
export -f do_install

################################################################################

if [[ ${options[only-cache]:-0} == 1 ]]; then
    exit 0
fi

################################################################################

set_target_package

case "${config:-default}" in
install-headers)
    log_command 'installing headers' -- do_install_headers
    ;;
default)
    log_command 'building' -- do_build
    log_command 'installing' -- do_install
    ;;
*)
    fatal "unknown configuration: $config"
    ;;
esac

