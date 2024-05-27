#!/usr/bin/env bash

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

source $PGT_HELPER
source_package_helpers

set -eo pipefail

# function to call another function if it exists
maybe_call_function() {
    local fn="$1"; shift
    ! type "$fn" >/dev/null 2>&1 \
        || "$fn" "$@"
}

# figure out our prefix from our arguments, then (maybe) un-finalize our
# toolchain (if it's already been finalized before)
prefix=$(get_current_prefix "$@") \
    || fatal "unable to determine toolchain prefix"
unfinalize_toolchain "$prefix"

# purely host tools
maybe_call_function extra_pre_host "$@"
build_package.sh "$@" pkg-config
build_package.sh "$@" libelf
build_package.sh "$@" elfio
build_package.sh "$@" zlib
build_package.sh "$@" zstd
build_package.sh "$@" binutils
build_package.sh "$@" gcc -chost
build_package.sh "$@" polyglot-host-tools
maybe_call_function extra_host "$@"

# core target libraries
maybe_call_function extra_pre_core "$@"
build_package.sh "$@" gcc -cspecs
build_package.sh "$@" target/boost-preprocessor -cinstall-headers
build_package.sh "$@" target/libc -cinstall-headers
build_package.sh "$@" target/udns -cinstall-headers
maybe_call_function extra_core_headers "$@"
build_package.sh "$@" gcc -ctarget
build_package.sh "$@" target/libc
build_package.sh "$@" target/udns
maybe_call_function extra_core_libraries "$@"

# extra target libraries
maybe_call_function extra_pre_target "$@"
build_package.sh "$@" target/libxcrypt
maybe_call_function extra_target "$@"

# and once everything has succeeded, (re-)finalize the toolchain
finalize_toolchain "$prefix"

