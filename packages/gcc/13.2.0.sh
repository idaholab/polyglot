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

#################################################################################

download_gcc()
{
    download_gcc_package '13.2.0'
}
download_gmp()
{
    download_gnu_package 'gmp' '6.3.0'
}
download_isl()
{
    download_package 'isl-0.18' \
        --checksum '13237a66fc623517fc570408b90a11e60eb6b4b9' \
        "https://libisl.sourceforge.io/isl-0.18.tar.xz"
}
download_mpc()
{
    download_gnu_package 'mpc' '1.3.1'
}
download_mpfr()
{
    download_gnu_package 'mpfr' '4.2.1'
}

################################################################################

source "$(find_package_script gcc modern)"

