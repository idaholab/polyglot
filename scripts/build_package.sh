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

################################################################################

# this contains a lot of the setup/tools we need for the rest of everything
source $PGT_HELPER

################################################################################

usage() {
    local p="$(basename "$0")"
    echo "Usage: $p [-h] [-v] [-o <option>=<value>] [-p <prefix>] \
            <package> [<version>]" | sed -r 's/ +/ /g'
}

help() {
    cat <<- EOF
	$(usage)
	
	FIXME: more info
	
	EOF
}

################################################################################

# parse our command line options
shortopts="ho:c:p:v"
longopts="help,option:,config:,prefix:,print-log,print-time,verbose"
tempargs=$(getopt -o $shortopts -l $longopts -- "$@") \
    || fatal "invalid arguments specified"
eval set -- "$tempargs"

declare print_time=0 print_log=0
export -A options=()
export prefix="." verbose=0 package version config=""

# handle our optional arguments
while :; do
    case "$1" in
    --help|-h)      help; exit 0 ;;
    --verbose|-v)   verbose=1; shift ;;
    --option|-o)    [[ $2 == *=* ]] \
                        && options[${2%%=*}]="${2#*=}" \
                        || options[$2]='1'
                    shift 2 ;;
    --config|-c)    config="$2"; shift 2 ;;
    --prefix|-p)    prefix="$(readlink -m "$2")"; shift 2 ;;
    --print-time)   print_time=1; shift ;;
    --print-log)    print_log=1; shift ;;
    --)             shift; break ;;
    *)              break ;;
    esac
done

# handle our positional arguments
case "$#" in
0)  print_fatal "no package name specified"; usage; exit 0 ;;
1)  package="$1" ;;
2)  package="$1"; version="$2" ;;
*)  print_fatal "invoked with too many arguments"; usage; exit 0 ;;
esac

################################################################################

#out_messages[stamp_exit]='\033[2m[package %b already built]\033[0m'
#out_messages[log_path]='\033[2m(logging to \033[4m%s\033[24m)\033[0m'
#out_messages[build_start]='\033[1m[building package %b]\033[0m'
#out_messages[log_time]='\033[2m(build completed in %02d:%02d)\033[0m'
#log_messages[build_start]='[BEGIN PACKAGE BUILD %s (%s)]'
log_messages[build_done]='[END PACKAGE BUILD (%s, %s seconds)]'

################################################################################

# pull in our package build environment helpers
source_package_helpers

################################################################################

# locate a script for this package/version, then turn that back into a version
package_script="$(find_package_script "$package" "$version")" || exit
export package_script
# see if we found a bare package; if not, and we weren't given a version,
# extract it from the resulting package script path
[[ $package_script == */$package.sh ]] \
    && version="" \
    || version="${version:-$(basename "$package_script" .sh)}"

# various versions of our package info, for printing/filenames/etc.
pkg_fancy="$(printf '%b%b%b' \
    "\033[35m$package\033[39m" \
    "${version:+ \033[32m$version\033[39m}" \
    "${config:+ \033[36m$config\033[39m}" \
)"
pkg_group="$(dirname "$package")"
pkg_basename="$(basename "$package")${version:+-$version}"
pkg_fullname="$pkg_basename${config:+-$config}"

# make variables for some non-package-specific, prefix-relative paths to allow
# more flexibility going forward (e.g. we can move them without breaking scripts
# because scripts depend on these variables instead of fixed paths)
export prefix_bin="$prefix/bin"
export prefix_localbin="$prefix_bin-local"
export prefix_extrabin="$prefix_bin-extra"
export prefix_include="$prefix/include"
export prefix_lib="$prefix/lib"
export prefix_libexec="$prefix/libexec"
export prefix_src="$prefix/src"
export prefix_var="$prefix/var"
export prefix_log="$prefix_var/log"
export prefix_stamp="$prefix_var/stamp"
export prefix_tmp="$prefix_var/cache"
export target_prefix="$prefix/target"

# also create some package-specific path variables (for similar reasons)
export src_dir="$prefix_src/$pkg_group/$pkg_basename"
export tmp_dir="$prefix_tmp/$pkg_group/$pkg_fullname"
export build_dir="$tmp_dir/build"
export log_file="$prefix_log/$pkg_group/$(_path_now)-$pkg_fullname.log"
export stamp_file="$prefix_stamp/$pkg_group/$pkg_fullname"

################################################################################

# henceforth, any top-level commands will break execution
set -eo pipefail

################################################################################

# check for our stamp file; if it already exists, we have nothing to do
if [ -e "$stamp_file" ]; then
    write_out '\033[2m[package %b already built]\033[0m' "$pkg_fancy"
    exit 0
fi

################################################################################

# add our tools (regardless of whether or not they exist yet) to the path
export PATH="$prefix_bin:$prefix_localbin:$PATH"

# cache original flags in case we're a target package
export SAVE_CPPFLAGS="$CPPFLAGS"
export SAVE_CFLAGS="$CFLAGS"
export SAVE_CXXFLAGS="$CXXFLAGS"
export SAVE_LDFLAGS="$LDFLAGS"
export SAVE_LDLIBS="$LDLIBS"

# declare our host-specific flags prior to sourcing platform scripts
export host_cpp host_cc host_cxx host_ld host_ar host_ranlib
export -a host_cflags=()
export -a host_cxxflags=()
export -a host_cppflags=()
export -a host_ldflags=()
export -a host_ldlibs=()
export -a configure_flags=()
export -a make_flags=()
export -a make_install_flags=()
export -a cmake_flags=()

# add our local prefix to host search locations
host_cppflags+=("-I$prefix/include")
host_ldflags+=("-L$prefix/lib")

################################################################################

################################################################################

# create our logfile
mkdir -p "$(dirname "$log_file")"
exec {log_fd}> >(_reformat >"$log_file") \
    || fatal "could not open logfile"

# print out some initial information
(( ! $print_log )) \
    || write_out '\033[2m(logging to \033[4m%s\033[24m)\033[0m' \
        "$(shorten_path "$log_file")"
write_out '\033[1m[building package %b]\033[0m' "$pkg_fancy"
write_log '[BEGIN PACKAGE BUILD %s (%s)]' "$pkg_group/$pkg_fullname" "$(_now)"

# record our start time
start_time=$(_epoch_now)

# pull in any host-specific information, then set this to build as a host
# package by default (target packages will fix this when needed)
source_all $(_locate_files PGT_DATA "scripts/helpers/host.sh")
set_host_package

# make sure our prefix is at least minimally provisioned
mkdir -p "$prefix"
mkdir -p "$prefix_include"
mkdir -p "$prefix_lib"
mkdir -p "$prefix_tmp"
mkdir -p "$prefix_src"

# also create directories required for the build
mkdir -p "$(dirname "$src_dir")"
mkdir -p "$build_dir"

# actually do the build via sourcing our script
source "$package_script"

# calculate how long our package build took
duration=$(($(_epoch_now)-start_time))

# log some final information
write_log '[END PACKAGE BUILD (%s, %d seconds)]' "$(_now)" "$duration"
(( ! $print_time )) \
    || write_out '\033[2m(build completed in %02d:%02d)\033[0m' \
        $((duration/60)) $((duration%60))

# and finally, exit successfully!
exit 0

