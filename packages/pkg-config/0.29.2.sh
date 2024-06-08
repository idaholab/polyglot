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

case $(get_host_type) in
darwin)
    # apparently pkg-config "forgets" to link against these... "neat"
    export LIBS="-framework CoreFoundation -framework Carbon"
    # also we can't use the Homebrew glib because it's not x86_64 for sure
    configure_flags+=("--with-internal-glib")
    ;;
esac

################################################################################

download_pkg_config()
{
    set -eo pipefail
    download_package 'pkg-config-0.29.2' \
        --signature '%.asc' --key "023A4420C7EC6914" \
        "https://pkgconfig.freedesktop.org/releases/pkg-config-0.29.tar.gz"
}

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[pkg-config]}" "$src_dir"
}
export -f do_extract

do_configure()
{
    set -exo pipefail
    cd "$build_dir"
    "$src_dir/configure" \
        "${configure_flags[@]}" \
        --prefix="$prefix" \
        --program-prefix=$target- \
        --disable-host-tool \
        --with-system-include-path="$target_prefix/include" \
        --with-system-library-path="$target_prefix/lib" \
        --with-pc-path="$target_prefix/lib/pkgconfig"
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
    [ -e "$prefix_localbin/$target-local-pkg-config" ] || {
        mkdir -p "$prefix_bin" "$prefix_localbin"
        ln -s "$prefix_bin/$target-pkg-config" \
            "$prefix_localbin/$target-local-pkg-config"
    }
}
export -f do_install

################################################################################

export -A files=()

if ! is_source_extracted; then
    files[pkg-config]="$(download_pkg_config)" \
        || fatal "failed to download pkg-config"
    log_command 'extracting' -- do_extract
    mark_source_extracted
fi
if needs_configured; then
    log_command 'configuring' -- do_configure
fi
log_command 'building' -- do_build
log_command 'installing' -- do_install
finalize_package

