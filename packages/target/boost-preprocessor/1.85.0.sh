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

GH_TAGS_URL="https://github.com/boostorg/preprocessor/archive/refs/tags"

download_boost_preprocessor()
{
    set -eo pipefail
    download_package 'boost-preprocessor-1.85.0' \
        "$GH_TAGS_URL/boost-1.85.0.tar.gz"
}

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[boost-preprocessor]}" "$src_dir"
}
export -f do_extract

do_install_headers()
{
    set -exo pipefail
    mkdir -p "$target_prefix/include"
    rsync -Lcrd --include='*.h' --include='*.hpp' \
        --include='*/' --exclude='*' \
        "$src_dir/include/" "$target_prefix/include/"
}
export -f do_install_headers

################################################################################

set_target_package

export -A files=()
if ! is_source_extracted; then
    files[boost-preprocessor]="$(download_boost_preprocessor)" \
        || fatal "failed to download boost-preprocessor"
    log_command 'extracting' -- do_extract
    mark_source_extracted
fi

case "${config:-default}" in
install-headers)
    log_command 'installing headers' -- do_install_headers
    finalize_package --keep-src
    ;;
*)
    fatal "unknown configuration: $config"
    ;;
esac

