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

export -a host_tools=( \
    modify_elf
    exar
    elf2macho
    brand_elf
)

################################################################################

tool_make()
{
    set -exo pipefail
    local tool="$1"
    local makefile="$(locate_file PGT_DATA "tools/$1/Makefile")" \
        || fatal "unable to locate Makefile for tool '$1'"
    shift
    make -f$makefile \
        $(locate_all_files PGT_DATA 'tools/common' '-I%s') \
        prefix="$prefix" \
        cachedir="$build_dir/$tool" \
        bin_prefix=$target- \
        "$@"
}
export -f tool_make

################################################################################

do_tool_build()
{
    set -exo pipefail
    local tool="$1"
    shift
    tool_make "$tool" "${make_flags[@]}" "$@"
}
export -f do_tool_build

do_tool_install()
{
    set -exo pipefail
    local tool="$1"
    shift
    tool_make "$tool" "${make_install_flags[@]}" install "$@"
}
export -f do_tool_install

do_build() {
    local tool
    for tool in "${host_tools[@]}"; do
        do_tool_build $tool
    done
}
export -f do_build

do_install() {
    local tool
    for tool in "${host_tools[@]}"; do
        do_tool_install $tool
    done
}
export -f do_install

################################################################################

if [[ ${options[only-cache]:-0} == 1 ]]; then
    exit 0
fi

################################################################################

log_command 'building' -- do_build
log_command 'installing' -- do_install

