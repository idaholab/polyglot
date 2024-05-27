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

# FIXME: this should be separated out differently so we don't have to do this to
# keep the build script environment from spilling over and "poisoning" ours...
unset _prog log_fd log_file
source $PGT_HELPER

################################################################################

prog="$(basename "$0")"

usage() { cat <<EOF
Usage: $prog [-s<url> -k<url/key>|-c<hash>] <output> <url> [<url> ...]
EOF
}

help() { cat <<EOF
$(usage)

Download a file. Supports checksum- and signature-based validation. On success,
prints the path to the downloaded file.

Positional arguments:
  <output>                  where to store the resulting file
  <url>                     URL to retrieve the file from

Optional arguments:
  -h/--help                 display this help message
  -s<>/--signature <url>    URL to retrieve the file signature from; may include
                            '%', which will be replaced by the URL being
                            validated before fetching
  -k<>/--key <url/key>      URL to retrieve the validation key from; may be
                            specified multiple times, resulting keys/keyring are
                            imported into a Polyglot-specific keyring
  -c<>/--checksum <hash>    expected checksum of the downloaded file

EOF
}

################################################################################

# parse our command line options
tempargs=$(getopt -o hvs:k:c:l: -l help,verbose,signature:,key:,checksum:,log: -- "$@") \
    || { usage >&2; exit 1; }
eval set -- "$tempargs"

declare -a keys=()
v=

# handle our optional arguments
while :; do
    case "$1" in
    --help|-h)      help; exit 0 ;;
    --signature|-s) signature="$2"; shift 2 ;;
    --key|-k)       keys+=("$2"); shift 2 ;;
    --checksum|-c)  checksum="$2"; shift 2 ;;
    --verbose|-v)   v=x; shift ;;
    --)             shift; break ;;
    *)              break ;;
    esac
done

# handle our positional arguments
case "$#" in
0)  usage_fatal "no output location specified" ;;
*)  output="$1"; shift
    declare -a urls=()
    while (( $# )); do
        [ -z "$1" ] || urls+=("$1")
        shift
    done
    ;;
esac

# sanity check our arguments
[ "$output" ] \
    || fatal "output path cannot be blank"
[ ${#urls[@]} -gt 0 ] \
    || fatal "no valid URLs provided"
[ -z "$checksum" -o -z "$signature" ] \
    || fatal "cannot combine --checksum/--signature"
[ -z "$signature" -o ${#keys[@]} -gt 0 ] \
    || fatal "--signature requires at least one --key argument"

set -o pipefail

################################################################################

_if_verbose() {
    if [ "$v" ]; then cat 1>&2; else cat >/dev/null; fi
}
_log() {
    printf "$@" | _if_verbose
}

_encode() { # text
    base64 -w0 <<<"$1" | tr '/' '?'
}
_decode() { # text
    tr '?' '/' <<<"$1" | base64 -d -w0
}

_key_path() { # url
    [ -z "$1" ] && echo "$PGT_DOWNLOADS/keys" \
                || echo "$PGT_DOWNLOADS/keys/$(_encode "$1")"
}
_signature_path() { # base-url
    [ -z "$1" ] && echo "$PGT_DOWNLOADS/signatures" \
                || echo "$PGT_DOWNLOADS/signatures/$(_encode "$1")"
}
_download_path() { # key-type, key
    [ -z "$2" ] && echo "$PGT_DOWNLOADS/by-$1" \
                || echo "$PGT_DOWNLOADS/by-$1/$(_encode "$2")"
}

_download() ( # output, url...
    (
        printf -- "> downloading '%s' to '%s'\n" "$2" "$1"
        set -eo pipefail
        mkdir -p "$(dirname "$1")"
        curl -fLo "$1" "$2" 2>&1 || rm -f "$1"
    ) | _if_verbose
    [ -f "$1" ]
)

gpg_home="$PGT_CACHE/gnupg"

_ensure_gpg_homedir() {
    [ -e "$gpg_home" ] && return 0
    (
        printf -- '> creating GPG home directory\n'
        set -eo pipefail
        mkdir -p "$gpg_home"
        chmod 0700 "$gpg_home"
        gpg --homedir="$gpg_home" --fingerprint
        mkdir -p "$gpg_home/ingested"
        cat >> "$gpg_home/gpg.conf" <<-EOF
		keyserver hkps://keys.openpgp.org/
		keyserver hkps://pgp.surf.nl
		EOF
    ) | _if_verbose \
    || fatal "unable to provision GPG homedir"
}

GPG_ARMOR_BEGIN="-----BEGIN PGP PUBLIC KEY BLOCK-----"

_gpg_ingest_keyfile() { # path-to-key, [url]
    # the basename is always an encoded url, so it's a valid unique key
    [ -e "$gpg_home/ingested/$(basename "$1")" ] && return 0
    [ -f "$1" ] || internal_fatal "keyfile doesn't exist"
    if grep -e "$GPG_ARMOR_BEGIN" "$1"; then
        # it's probably exported things? let's just import it
        (
            printf -- "> ingesting GPG keyfile '%s'\n" "$1"
            set -eo pipefail
            gpg --homedir="$gpg_home" --import <"$1"
        ) | _if_verbose \
        || fatal "importing key(s) failed: ${2:-$1}"
    else
        # maybe it's a keyring?
        (
            printf -- "> ingesting GPG keyring '%s'\n" "$1"
            set -eo pipefail
            gpg --homedir="$gpg_home" --no-default-keyring --keyring="$1" \
                    --export --armor | gpg --homedir="$gpg_home" --import
        ) | _if_verbose \
        || fatal "importing keyring failed: ${2:-$1}"
    fi
    ln -s "$1" "$gpg_home/ingested/$(basename "$1")"
}

_gpg_receive_key() { # public-key
    if ! gpg --homedir="$gpg_home" -k "$1" >/dev/null 2>&1; then
        (
            printf -- "> receiving GPG key '%s'\n" "$1"
            set -eo pipefail
            gpg --homedir="$gpg_home" --recv-keys "$1"
        ) | _if_verbose \
        || fatal "receiving public key failed: $1"
    fi
}

_check_signature() { # file, signature
    _log "> verifying GPG signature '%s' of file '%s'\n" "$2" "$1"
    gpg --homedir="$gpg_home" --verify "$2" "$1" 2>&1 | _if_verbose
}

_fetch_and_check_signature() { # temp, url, signature
    local sig_url="$3" sig_path
    [[ "$sig_url" =~ ^(.*)%(.*)$ ]] \
        && sig_url="${BASH_REMATCH[1]}$2${BASH_REMATCH[2]}"
    sig_path="$(_signature_path "$sig_url")"
    { [ -e "$sig_path" ] || _download "$sig_path" "$sig_url"
    } && _check_signature "$1" "$sig_path"
}

_get_all_gpg_keys() {
    # make sure that our gpg homedir is valid
    _ensure_gpg_homedir || exit
    # download and ingest all the specified keyfiles
    for key in "${keys[@]}"; do
        if [[ "$key" == *://* ]]; then
            key_path="$(_key_path "$key")"
            if ! [ -e "$key_path" ] && ! _download "$key_path" "$key"; then
                continue
            fi
            _gpg_ingest_keyfile "$key_path" "$key" || exit
        elif [[ "$key" =~ ^[0-9A-Fa-f]{8,}$ ]]; then
            _gpg_receive_key "$key" || exit
        else
            fatal "unknown key format: $1"
        fi
    done
}

_download_with_signature() {
    # walk our URLs first to see if we've already downloaded the file
    for url in "${urls[@]}"; do
        temp="$(_download_path url "$url")"
        if [ -e "$temp" ]; then
            _fetch_and_check_signature "$temp" "$url" "$signature" \
                && return
            rm -f "$temp" "$sig_path"
        fi
    done
    # none are already downloaded, try to get each URL
    for url in "${urls[@]}"; do
        temp="$(_download_path url "$url")"
        if _download "$temp" "$url"; then
            _fetch_and_check_signature "$temp" "$url" "$signature" \
                && return
            rm -f "$temp" "$sig_path"
        fi
    done
    # if we reached this point, no URLs are valid
    unset temp
}

_download_with_checksum() {
    # figure out what sort of checksum we have, and where to put the file
    hash_algo=$(_detect_hash_algorithm "$checksum") || return 1
    temp="$(_download_path $hash_algo "$checksum")"
    # see if we need to download our file or not
    if [ -e "$temp" ]; then
        # we don't, decode its link to see where we got it
        url="$(_decode "$(basename "$(readlink "$temp")")")"
        _log "> downloaded previously from '%s'\n" "$url"
        return
    # we do still need to download it
    else
        # try to download each of our urls, then check the hash
        for url in "${urls[@]}"; do
            url_path="$(_download_path url "$url")"
            if ! [ -e "$url_path" ] && ! _download "$url_path" "$url"; then
                continue
            fi
            _log "> checking file hash of '%s'\n" "$url_path"
            if check_file_hash "$hash_algo" "$url_path" "$checksum"; then
                _log "> hash matches, linking to '%s'\n" "$temp"
                mkdir -p "$(dirname "$temp")"
                [ -h "$temp" ] || ln -s "$url_path" "$temp"
                return
            else
                _log '> invalid hash, removing file\n'
                rm -f "$url_path"
            fi
        done
    fi
    # if we reached this point, we failed all URLs, so clear the path
    unset temp
}

_simple_download() {
    # first see if any of our files were downloaded successfully
    for url in "${urls[@]}"; do
        temp="$(_download_path url "$url")"
        [ ! -e "$temp" ] || return
    done
    # nope, work through our urls trying to download each
    for url in "${urls[@]}"; do
        temp="$(_download_path url "$url")"
        if _download "$temp" "$url"; then
            return
        fi
    done
    # if we reached this point, we failed all URLs, so clear the path
    unset temp
}

################################################################################

{
    printf -- '= FILE DOWNLOAD PLAN =======================================\n'
    printf '| output location: %s\n' "$output"
    printf '| possible download URLs:\n'
    for url in "${urls[@]}"; do
        printf '|  - %s\n' "$url"
    done
    if [ "$checksum" ]; then
        printf '| expected checksum: %s\n' "$checksum"
    fi
    if [ "$signature" ]; then
        printf '| signature URL: %s\n' "$signature"
        printf '| possible signing keys:\n'
        for key in "${keys[@]}"; do
            printf '|  - %s\n' "$key"
        done
    fi
} | _if_verbose

# try to download our file with our given parameters
if [ "$signature" ]; then
    _get_all_gpg_keys || exit
    _download_with_signature
elif [ "$checksum" ]; then
    _download_with_checksum
else
    _simple_download
fi

# if we didn't end up with a valid file at the end, we're done
if [ ! "$temp" -o ! -e "$temp" ]; then
    fatal "failed to download file"
fi

# if we were given a directory to write to, figure out the filename
if [ -d "$output" -o "${output:${#output}-1}" == '/' ]; then
    output="$output/$(basename "$url")"
    _log "> treating output path as directory; saving to '%s'\n" "$output"
fi

# link our downloaded file to our output location
mkdir -p "$(dirname "$output")"
if [ -h "$output" ]; then
    unlink "$output" \
        || fatal "output path exists as symlink but unable to unlink"
elif [ -e "$output" ]; then
    rm -f "$output" \
        || fatal "output path exists but unable to remove"
fi
_log '> linking to output file\n'
ln -s "$temp" "$output"

# cool, success
echo "$output"
exit 0

