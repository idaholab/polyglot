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

export script="${options[script]}"
export script_pwd="${options[pwd]}"
export script_args="${options[args]}"
export script_real="$({ cd "$script_pwd"; realpath "$script"; })"

cat >"$prefix/rebuild.sh" <<EOF
#!/usr/bin/env bash
$script_real $script_args "\$@"
EOF
chmod +x "$prefix/rebuild.sh"

finalize_package

