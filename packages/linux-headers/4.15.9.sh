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

download_linux()
{
    # we could use the signature, but it's a signature for the .tar, not the
    # .tar.xz, and we currently aren't set up to handle that kind of thing,
    # soooo... just use the SHA-1, I guess?
    download_package 'linux-4.15.9' \
        --checksum '1bb8d78c66c9225976dfeb793545eae453140609' \
        'https://mirrors.edge.kernel.org/pub/linux/kernel/v4.x/linux-4.15.9.tar.xz'
}

################################################################################

source "$(find_package_script linux-headers modern)"

