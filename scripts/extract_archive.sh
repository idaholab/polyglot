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

prog="$(basename "$0")"

usage() {
	echo "Usage: $prog [-h] <archive> [<output>]"
}

help() {
    cat <<-EOF
	$(usage)
	
	Extract an archive generically; pretends that archives with no singular
	top-level directory aren't the literal worst thing ever and fixes them.
	
	Positional arguments:
	  <archive>                 archive to extract
	  <output>                  where to extract files
	
	Optional arguments:
	  -h/--help                 display this help message
	
	EOF
}

_reformat() {
    [ -t 1 ] && cat -u || sed -ure $'s/\033\\[[^m]+m//g' \
                                -e 's/^\r//' -e 's/\r/\n/g'
}
_write_fail() {
    _reformat >&2
}
fail() {
    printf '%s: %b\n' "$prog" "$1" | _write_fail
    exit 1
}
internal_fail() {
    printf '\033[1;31m[INTERNAL FAILURE]\033[0m %s: %b\n' \
            "$prog" "$1" | _write_fail
    exit 1
}
usage_fail() {
    printf '%s: %b\n' "$prog" "$1" | _write_fail
    usage >&2
    exit 1
}

which() {
    command which "$@" >/dev/null 2>&1
}

_python_hash()
{ # algorithm, path
    which python3 \
        || fail 'cannot use python hashing fallback, `python3` not in path'
    python3 -- - "${2:--}" <<-EOF
		import hashlib, sys
		
		def error(msg):
		    print(f'${FUNCNAME}: {msg}', file=sys.stderr)
		    sys.exit(1)
		
		if len(sys.argv) != 2:
		    error('not enough arguments')
		
		def do_hash(fh):
		    h = hashlib.${1:-sha1}()
		    while (b := fh.read(10240)):
		        h.update(b)
		    print(h.hexdigest())
		
		try:
		    if sys.argv[1] == '-':
		        do_hash(sys.stdin)
		    else:
		        with open(sys.argv[1], 'rb') as fh:
		            do_hash(fh)
		except Exception as exc:
		    error(str(exc))
	EOF
}

_only_hash() { # (reads from stdin)
    sed -nr $'s/^([A-Fa-f0-9]+)[ \t].*$/\\1/p'
}

_generic_shasum()
{ # algorithm-as-number, path
    if which sha${1:-1}sum; then
        command sha${1:-1}sum "${2:--}" | _only_hash
    elif which shasum; then
        shasum -a ${1:-1} "${2:--}" | _only_hash
    else
        _python_hash sha${1:-1} "$1"
    fi
}

md5sum()
{ # path
    if which md5sum; then
        command md5sum "${1:--}" | _only_hash
    elif which md5; then
        md5 -q "${1:--}"
    else
        _python_hash md5 "$1"
    fi
}
sha1sum()   { _generic_shasum 1 "$1"; }
sha224sum() { _generic_shasum 224 "$1"; }
sha256sum() { _generic_shasum 256 "$1"; }
sha384sum() { _generic_shasum 384 "$1"; }
sha512sum() { _generic_shasum 512 "$1"; }

################################################################################

[ -d "${HOMEBREW_PREFIX:-/dev/null}/opt/gnu-getopt" ] \
    && export PATH="$HOMEBREW_PREFIX/opt/gnu-getopt/bin:$PATH"
[ -d "${HOMEBREW_PREFIX:-/dev/null}/opt/grep" ] \
    && export PATH="$HOMEBREW_PREFIX/opt/grep/libexec/gnubin:$PATH"

# we need GNU grep/getopt--make sure what's in the path looks right
{ which grep && grep --version 2>&1 | grep "GNU grep" >/dev/null >&1; } \
    || fail "GNU grep not found"
{ which getopt && getopt --version 2>&1 | grep -qiEe "\<util-linux\>"; } \
    || fail "GNU getopt not found"

# parse our command line options
tempargs=$(getopt -o hl: -l help,log: -- "$@") \
    || usage_fail "invalid arguments specified"
eval set -- "$tempargs"

declare -a keys=()

# handle our optional arguments
while :; do
    case "$1" in
    --help|-h)      help; exit 0 ;;
    --log|-l)       log_file="$2"; shift 2 ;;
    --)             shift; break ;;
    *)              break ;;
    esac
done

# either dup stdout to be our logfile, or open the given file
if [ -z "$log_file" ]; then
    exec {log_fd}>&1
else
    exec {log_fd}>>"$log_file"
fi

# handle our positional arguments
case "$#" in
0)  usage_fail "no archive specified" ;;
1)  archive="$1" ;;
2)  archive="$1"; output="$2" ;;
*)  usage_fail "invoked with too many arguments" ;;
esac

# sanity check what we were given
[ -e "$archive" ] || fail "archive does not exist"

################################################################################

_filter_root_items() { # listing
    sed -rn 's|^([^/]+).*|\1|p' | sort -u
}

_tar_get_items() { # tarfile
    tar tf "$1"
}

_tar_basename() {
    basename "$1" | sed -r 's/\.(tar|tar\..*z|t.*z)$//'
}

_tar_extract() { # tarfile
    tar xvf "$1" 2>&1
}

_zip_get_items() { # zipfile
    unzip -Z1 "$1"
}

_zip_extract() { # zipfile
    unzip -o "$1" 2>/dev/null | tail -n+2 | sed -rn 's/^.+:\s*(.+)/\1/p'
}

################################################################################

case "$archive" in
*.tar|*.tar.*z*|*.t*z*)
    which tar || fail "tar not in path"
    extractor=_tar_extract
    lister=_tar_get_items
    base="$(_tar_basename "$1")"
    ;;
*.zip)
    which unzip || fail "unzip not in path"
    extractor=_zip_extract
    lister=_zip_get_items
    base="$(basename "$1" .zip)"
    ;;
*)
    fail "unsupported archive format"
    return 1
    ;;
esac

echo $archive $base

# figure out where we're putting our listing
checksum="$(sha256sum "$1")"
listing="$PGT_CACHE/archive_listings/$checksum"

mkdir -p "$(dirname "$listing")"
# generate the listing if we don't have one cached
if [ ! -e "$listing" ]; then
    $lister "$1" > "$listing"
fi

# figure out how we're extracting this based on whether there's a singular
# common root directory in the archive (most tarfiles) or not (some zips)
if [ "$(_filter_root_items <"$listing" | wc -l)" -ne 1 ]; then
    rootdir=''
    output="${output:-$base}"
else 
    rootdir="$(_filter_root_items <"$listing" | head -n1)"
    output="${output:-$rootdir}"
fi

# if we are supposed to extract into the dir, tweak our output path
if [ -d "$output" -a "${output:${#output}-1}" == '/' ]; then
    output="$output/$base"
fi

# make sure our output directory doesn't already exist
[ -e "$output" ] && \
    fail "output dir exists: $output"

# create a temp dir for us to extract things into
mkdir -p "$PGT_CACHE/tmp"
temp="$(mktemp -d "$PGT_CACHE/tmp/polyglot-extract-XXXXXXXXXXXX")"
pushd "$temp" >/dev/null 2>&1
# extract our archive there, then return
$extractor "$1"
popd >/dev/null 2>&1
# if the archive has a root dir, just move that to our output path
if [ -n "$rootdir" ]; then
    mv "$temp/$rootdir" "$output"
# otherwise, create the output dir and move everything from the temp folder
# over into the new output dir
else
    mkdir -p "$output"
    # XXX: because bash/zsh doesn't include dotfiles in '*', we use find...
    find "$temp" -mindepth 1 -maxdepth 1 -exec mv \{\} "$output" \;
fi
# and finally remove the temp dir
rm -rf "$temp"

