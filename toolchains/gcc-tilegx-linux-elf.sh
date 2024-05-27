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

set -- \
    -otarget=tilegx-linux-elf \
    -ogcc-target=tilegx-unknown-linux \
    "$@"

extra_pre_core() {
    build_package.sh "$@" linux-headers 4.15.9
}

gcc_script=$(locate_file PGT_DATA "toolchains/generic/gcc-11.1.0.sh") || exit
source $gcc_script

