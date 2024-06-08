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

get_current_target() {
    # if we have an options associative array with a target, use that
    if [ "${options[target]}" ]; then
        echo "${options[target]}"
        return 0
    fi
    # scan through our arguments looking for options
    local -a options=()
    while (( $# )); do
        case "$1" in
        --option=*) options+=("${1:9:${#1}}"); shift ;;
        --option)   options+=("$2"); shift 2 ;;
        --*)        shift ;;
        -*o*)       options+=("$(sed -r 's/^-[^o]*o//' <<<"$1")"); shift ;;
        -*o)        options+=("$2"); shift 2 ;;
        *)          shift ;;
        esac
    done
    # now scan through our options looking for targets
    local opt target
    for opt in "${options[@]}"; do
        if [[ "$opt" == target=* ]]; then
            target="${opt:7:${#opt}}"
        fi
    done
    # if we found any, print out the last one we found
    [ "$target" ] && echo "$target"
}

get_current_prefix() {
    # scan through our arguments looking for options
    local prefix
    while (( $# )); do
        case "$1" in
        --prefix=*) prefix="${1:9:${#1}}"; shift ;;
        --prefix)   prefix="$2"; shift 2 ;;
        --*)        shift ;;
        -*p*)       prefix="$(sed -r 's/^-[^p]*p//' <<<"$1")"; shift ;;
        -*p)        prefix="$2"; shift 2 ;;
        *)          shift ;;
        esac
    done
    # if we found any, print out the last one we found
    [ "$prefix" ] && echo "$prefix"
}

find_package_script() { # package, [version]
    # if we were given a version, just try to locate that specific script
    if [ "$2" ]; then
        _locate_file PGT_DATA "packages/$1/$2.sh" \
            || fatal "script for '$1-$2' not found"
    elif _locate_file PGT_DATA "packages/$1.sh"; then
        return
    else
        local script
        # this is a seemingly-super-complicated way to handle finding the
        # script with the highest version number, but it actually is pretty
        # straightforward: we tag each script with its extracted version number
        # and a -descending- index (so that the later paths have lower numbers
        # attached to them), then reverse sort all of them using human numeric
        # sorting (which makes the versions sort properly) which results in the
        # first instance of the higest version number being the first line
        script=$({
            local i=0;
            for path in $(_locate_files PGT_DATA "packages/$1"); do
                # convert each path to '<version> <index> <path>' format
                find "$path" -mindepth 1 -maxdepth 1 -iname '*.sh' \
                    | sed -r 's|^(.*/(.*)\.sh)|\2 '$((999-i))' :\1|'
                i=$((i+1))
            done
        } | sort -rh | head -n1)
        # if we didn't find any scripts, error out
        [ "$script" ] || fatal "no scripts for package \`$1' found"
        # otherwise output the script with the lead tagged info cut off
        echo "${script#*:}"
    fi
}
export -f find_package_script

set_host_package()
{
    export CPPFLAGS="${host_cppflags[@]} $CPPFLAGS"
    export CFLAGS="${host_cflags[@]} $CFLAGS"
    export CXXFLAGS="${host_cxxflags[@]} $CXXFLAGS"
    export LDFLAGS="${host_ldflags[@]} $LDFLAGS"
    export LDLIBS="${host_ldlibs[@]} $LDLIBS"
    [ ! "$host_cpp" ] || export CPP="$host_cpp"
    [ ! "$host_cc" ] || export CC="$host_cc"
    [ ! "$host_cxx" ] || export CXX="$host_cxx"
    [ ! "$host_ld" ] || export LD="$host_ld"
    [ ! "$host_ar" ] || export AR="$host_ar"
    [ ! "$host_ranlib" ] || export RANLIB="$host_ranlib"
}
export -f set_host_package

set_target_package()
{
    export CPPFLAGS="$SAVE_CPPFLAGS"
    export CFLAGS="$SAVE_CFLAGS"
    export CXXFLAGS="$SAVE_CXXFLAGS"
    export LDFLAGS="$SAVE_LDFLAGS"
    export LDLIBS="$SAVE_LDLIBS"
    unset CPP CC CXX LD AR RANLIB
}
export -f set_target_package

find_package_files() { # subpath, [package, [version]]
    # use our current package/version if one wasn't provided
    local package="${2:-$package}"
    local version="${3:-$version}"
    local base="packages/$package"
    if [[ "$1" == */%/* ]]; then
        # our subpath has a version insert, so it's a single lookup
        _locate_files PGT_DATA "$base/${1/\%/$version}"
    else
        # try to locate any of several paths
        local r=1
        ! _locate_files PGT_DATA "$base/$version/$1" || r=0
        ! _locate_files PGT_DATA "$base/$1" || r=0
        return $r
    fi
}
export -f find_package_files

is_source_extracted() {
    if [ ! -e "$src_dir/.extracted" ]; then
        [ ! -e "$src_dir" ] \
            || command_timer \
                -i2 -m 'cleaning up source' \
                -E '/dev/null' -O '/dev/null' \
                -- rm -rf "$src_dir"
        return 1
    else
        return 0
    fi
}
export -f is_source_extracted

mark_source_extracted() {
    if [ ! -e "$src_dir/.extracted" ]; then
        mkdir -p "$src_dir"
        touch "$src_dir/.extracted"
    fi
}
export -f mark_source_extracted

needs_configured() {
    [ ! -e "$build_dir/Makefile" ]
}
export -f needs_configured

write_stamp() {
    mkdir -p "$(dirname "$stamp_file")"
    touch "$stamp_file"
}
export -f write_stamp

clean_tmp() {
    command_timer \
        -i2 -m 'cleaning up temporary files' \
        -E '/dev/null' -O '/dev/null' \
        -- rm -rf "$tmp_dir"
}
export -f clean_tmp

clean_src() {
    command_timer \
        -i2 -m 'cleaning up extracted source' \
        -E '/dev/null' -O '/dev/null' \
        -- rm -rf "$src_dir"
}
export -f clean_src

finalize_package()
{
    local no_stamp keep_src keep_tmp
    while (( $# )); do
        case "$1" in
        --no-stamp)     no_stamp=yes ;;
        --keep-tmp|-t)  keep_tmp=yes ;;
        --keep-src|-s)  keep_src=yes ;;
        *) fatal "unknown argument to finalize_package(): $1" ;;
        esac
        shift
    done
    [ "$no_stamp" ] || write_stamp
    [ "$keep_tmp" ] || clean_tmp
    [ "$keep_src" ] || clean_src
    return 0
}
export -f finalize_package

is_toolchain_finalized() { # toolchain
    [ -e "$PGT_TOOLCHAINS/$1/var/stamp/final" ]
}
export -f is_toolchain_finalized

unfinalize_toolchain() { # prefix
    rm -f "$1/var/stamp/final"
}
export -f unfinalize_toolchain

finalize_toolchain() { # prefix
    #mkdir -p "$1/var/stamp"
    touch "$1/var/stamp/final"
}
export -f finalize_toolchain

