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

[ ! "$_COMMON_SH_INCLUDED" ] || return
export _COMMON_SH_INCLUDED=yes

## TIMESTAMPS ##################################################################

_now() { date '+%Y-%m-%d %H:%M:%S'; }
_path_now() { date '+%Y%m%d%H%M%S'; }
_epoch_now() { date '+%s'; }
export -f _now _path_now _epoch_now

## PATH HELPERS ################################################################

source_all() {
    local -a scripts=()
    local script
    while (( $# )); do
        case "$1" in
        --) shift; break ;;
        *) scripts+=("$1"); shift ;;
        esac
    done
    for script in "${scripts[@]}"; do
        source "$script" "$@"
    done
}
export -f source_all

source_package_helpers() {
    source_all $(_locate_files PGT_DATA 'scripts/helpers/package.sh') "$@"
}
export -f source_package_helpers

shorten_path() {
    if [ -z "$1" ]; then
        echo "."
        return
    fi
    local path="$(readlink -m "$1")"
    local cur="$(readlink -m .)"
    if [[ $path == $cur/* ]]; then
        echo ".${path:${#cur}}"
        return
    fi
    local home="$(readlink -m ~)"
    if [[ $path == $home/* ]]; then
        echo "~${path:${#home}}"
        return
    fi
    echo "$path"
}
export -f shorten_path

_locate_files() {
    local dirs d p r=1
    [[ "$1" == *[/:.]* ]] && dirs="$1" || dirs="${!1}"
    for d in ${dirs//:/ }; do
        [ "$d" ] || continue
        for p in $(echo $d/$2 2>/dev/null || true); do
            if [ -e $p ]; then
                [ "$3" ] && printf -- "$3" "$p" || printf -- '%s\n' "$p"
                r=0
            fi
        done
    done
    return $r
}
export -f _locate_files

_locate_file() (
    set -o pipefail
    _locate_files "$@" | head -n1
)
export -f _locate_file

locate_file() {
    _locate_file "$@" \
        || fatal "unable to locate $2 relative to $1"
}
export -f locate_file

locate_all_files() {
    _locate_files "$@" \
        || fatal "unable to locate $2 relative to $1"
}
export -f locate_all_files

sanitize_path_set() {
    sed -re's|/:|:|g' -e's/:*$/\n/'
}

add_to_paths_in_set() {
    sed -re's/^:+//' -e's/:+$//' -e"s/(:|$):*/${1//\//\\\/}\\1/g"
}

ensure_all_set_exists() {
    _locate_files "$1" '' '%s:' | sanitize_path_set \
        || { [ "$2" ] && echo "$2"; }
}

## LOGGING #####################################################################

# these will be actually defined in proper scripts
export log_file logfd

_reformat() {
    [ -t 1 ] && cat -u || sed -ur $'s/\033\\[[^m]+m//g'
}
export -f _reformat

_fancy_linebreaks() {
    sed -r $'s/^ */  \033[2m|\033[0m /'
}
export -f _fancy_linebreaks

export _prog=
print_fatal() {
    {
        printf '\033[31m%s: %b\033[0m\n' "${_prog:-$(basename "$0")}" "$1"
        {
            [ "$2" ] && printf '%b\n' "$2"
            [ "$log_file" ] && printf 'see log for more detail: %s\n' \
                                      "$(shorten_path "$log_file")"
        } \
        | _fancy_linebreaks
    } | _reformat >&2
    if [ "${log_fd}" ]; then
        {
            printf '[FATAL] %b (%s)\n' "$1" "$(_now)"
            [ "$2" ] && printf  '%b\n' "$2" | sed 's/^/  | /'
        } >&$log_fd
    fi
}
export -f print_fatal

fatal() {
    print_fatal "$@"
    exit 1
}
export -f fatal

internal_fatal() {
    printf '\r\033[1;31m[INTERNAL FAILURE]\033[0m '
    fatal "$@"
}
export -f internal_fatal

usage_fatal() {
    print_fatal "$@"
    usage >&2
    exit 1
}
export -f usage_fatal

warn() {
    {
        printf '\033[33m%s: %b\033[0m\n' "$(basename "$0")" "$1"
        [ "$2" ] && printf '%b\n' "$2" | _fancy_linebreaks
    } | _reformat >&2
    if [ "${log_fd}" ]; then
        {
            printf '[WARN] %b (%s)\n' "$1" "$(_now)"
            [ "$2" ] && printf  '%b\n' "$2" | sed 's/^/  | /' 
        } >&$log_fd
    fi
}
export -f warn

write_out() {
    local msg="$1"; shift
    printf "$msg\n" "$@" | _reformat
}
export -f write_out

write_log() {
    local msg="$1"; shift
    printf "$msg\n" "$@" | >&$log_fd
}
export -f write_log

################################################################################

get_host_type() {
    [ -z "$_host_type" ] || { echo $_host_type; return; }
    case "$(uname -s)" in
    Darwin) _host_type=darwin ;;
    Linux)  [ -e /etc/os-release ] \
                && _host_type=$(. /etc/os-release; echo ${ID:-linux}) \
                || _host_type=linux ;;
    *)      _host_type=unknown ;;
    esac
    echo $_host_type
}
export -f get_host_type
export _host_type

################################################################################

check_for_prog_generic() ( # command, [token], [advice]
    set -Eeo pipefail
    eval local -a args=("$1")
    local token="$2"
    shift 2
    # verify the program exists
    which "${args[0]}" >/dev/null 2>&1 \
        || fatal "program '${args[0]}' is not in \$PATH" "$*"
    # if we were given a token to check for, do that
    if [ "$token" ]; then
        # verify the token is present in the executed command
        { command "${args[@]}" 2>&1 || true; } | grep -qiEe "$token" \
            || fatal "command '${args[*]}' output invalid" "$*"
    # we weren't given a token to look for, but we have a real command, so try
    # to run that to make sure it works
    elif (( ${#args[@]} > 1 )); then
        # verify the command to check executes
        command "${args[@]}" >/dev/null 2>&1 \
            || fatal "command '${args[*]}' failed" "$*"
    fi
    return 0
)
export -f check_for_prog_generic

_get_prog_advice() { cat <<EOF
    local advice='';
    local -a args=();
    while (( \$# )); do
        case "\$1" in
        --$1=*) advice="\${1:$((${#1}+3)):\${#1}}" ;;
        --*=*) ;;
        *) args+=("\$1") ;;
        esac;
        shift;
    done;
    set -- "\${args[@]}";
    unset args;
    if [ -z "\$advice" ]; then
        check_for_prog_generic "\$@";
        return;
    fi;
EOF
}
export -f _get_prog_advice

check_for_prog() {
    local fn=check_for_prog_$(get_host_type)
    if type $fn >/dev/null 2>&1; then
        $fn "$@"
    else
        check_for_prog_generic "$@"
    fi
}
export -f check_for_prog

check_for_prog_darwin() {
    eval $(_get_prog_advice darwin)
    eval local -a tool=($1)
    local prog="${tool[0]}" pkg="${advice%:*}" bindir="${advice##*:}" path
    if [ "$HOMEBREW_PREFIX" ]; then
        if ! check_for_prog_generic "$@" >/dev/null 2>&1; then
            if [ -d "$HOMEBREW_PREFIX/opt/$pkg" ]; then
                path="$HOMEBREW_PREFIX/opt/$pkg/$bindir"
                if [ ! -e "$path/$prog" ]; then
                    path="\$HOMEBREW_PREFIX/opt/$pkg/$bindir/$prog"
                    fatal "Incomplete Homebrew package: \033[35m$pkg\033[0m" \
                          "path does not exist: \033[35m$path\033[0m"
                fi
                if [ "$path/$prog" != "$(which $prog 2>/dev/null)" ]; then
                    PATH="$path:$PATH"
                fi
            fi
            check_for_prog_generic "$1" "$2" \
                "To fix, run: \033[35mbrew install $pkg\033[0m"
        fi
    else
        check_for_prog_generic "$1" "$2" \
            "We suggest installing Homebrew! (see https://brew.sh/)\n" \
            "Then, to fix, run: \033[35mbrew install $pkg\033[0m"
    fi
}
export -f check_for_prog_darwin

check_for_prog_debian() {
    eval $(_get_prog_advice debian)
    check_for_prog_generic "$1" "$2" \
        "To fix, run: \033[35mapt install -y $advice\033[0m"
}
export -f check_for_prog_debian

check_for_prog_ubuntu() {
    eval $(_get_prog_advice ubuntu)
    check_for_prog_generic "$1" "$2" \
        "To fix, run: \033[35mapt install -y $advice\033[0m"
}
export -f check_for_prog_ubuntu

check_for_prog_fedora() {
    eval $(_get_prog_advice fedora)
    check_for_prog_generic "$1" "$2" \
        "To fix, run: \033[35mdnf install -y $advice\033[0m"
}
export -f check_for_prog_fedora

check_for_prog_alpine() {
    eval $(_get_prog_advice alpine)
    check_for_prog_generic "$1" "$2" \
        "To fix, run: \033[35mapk add $advice\033[0m"
}
export -f check_for_prog_alpine

check_for_gnu_prog() {
    eval local -a args=("$1")
    local p="${args[0]}"
    check_for_prog "$@" \
        || fatal "$p does not seem to be GNU $p, cannot continue"
}
export -f check_for_gnu_prog

wrap_program_with_check() {
    local name="$1" cmd="$2" token
    shift 2
    if [[ "$1" != --* ]]; then
        token="$1"
        shift
    fi
    cat <<EOF
        $name() {
            check_for_prog '$cmd' '$token' $* || return;
            unset $name;
            command $name "\$@";
        };
        export -f $name
EOF
}

################################################################################

find_shell_vars() {
    grep -oE '\$([A-Za-z_][A-Za-z0-9_]*|\{[#!]?[A-Za-z_][A-Za-z0-9_]*)' \
        | sed -r 's/\$(\{[#!]?)?//'
}
export -f find_shell_vars

log_command() {
    # the message is always first
    local msg="$1"
    shift
    # try to locate a split between options to the timer and the command
    local -a opts=()
    while [ $# -gt 0 -a ! "$1" == -- ]; do
        opts+=("$1")
        shift
    done
    # okay, if we reached the end without finding a '--', just undo what we did
    # and assume it's all the command
    if (( ! $# )); then
        set -- "${opts[@]}"
        opts=()
    else
        shift
    fi
    if (( ${verbose:-0} )); then
        {
            printf $'[%b]\\n' "$msg"
            printf $'(running command `%s\')\\n' "$*"
            eval "$@" 2>&1
        } \
        | tee >(cat -u >&$log_fd)
    else
        {
            printf '> %b (%s)\n' "$1" "$(_now)"
            printf '> running command: %s\n' "$2"
            printf -- '--------------------------------\n'
        } >&$log_fd
        local ret=0
        local e=0
        if [ -o errexit ]; then
            e=1; set +e
        fi
        local -a env_args=()
        local -a names=()
        # if what we're calling is a function, then let's look for variables we
        # might be using in case we can copy stuff into our nested environment
        if [ $(type -t "$1") == function ]; then
            local attrs decl
            # extract any references to variables from the function definition
            for name in $(type "$1" | find_shell_vars); do
                # bash-style way of getting the attributes of a deref'd name
                attrs="${!name@a}"
                # if this is an exported array/association, let's try to copy it
                # into the environment (since it's not propagated normally)
                if { [[ $attrs == *a* ]] || [[ $attrs == *A* ]]; } \
                        && [[ $attrs == *x* ]]; then
                    # make sure we've not already grabbed this variable
                    if ! [[ " ${names[@]} " =~ " $name " ]]; then
                        # try to get the variable definition via set
                        if decl="$(set | grep -E "^$name=")"; then
                            env_args+=("-e$attrs:$decl")
                        fi
                        # mark this one as passed in
                        names+=("$name")
                    fi
                fi
            done
        fi
        # actually run the command via timer
        command_timer.py \
            -l -i2 -m "$msg" \
            -f ">&$log_fd" \
            -f "2>&1" \
            "${env_args[@]}" \
            "${opts[@]}" -- "$@"
        ret=$?
        (( $e )) && set -e
        {
            printf -- '--------------------------------\n'
            printf '> command exited with status %d (%s)\n' "$ret" "$(_now)"
        } >&$log_fd
        if (( $ret != 0 )); then
            fatal "subcommand exited with status $ret"
        fi
    fi
}
export -f log_command

################################################################################

export GNU_BASE_URL='https://ftp.gnu.org/gnu'
export GNU_KEYRING_URL="$GNU_BASE_URL/gnu-keyring.gpg"

download_gcc_package() {
    command_timer.py -L -i2 -m "downloading gcc-$1" -d1 -f "2>&$log_fd" \
    -- download_file.sh \
        --key "$GNU_KEYRING_URL" \
        --signature '%.sig' \
        "$PGT_DOWNLOADS/by-package/" \
        "$GNU_BASE_URL/gcc/gcc-$1/gcc-$1.tar.xz" \
        "$GNU_BASE_URL/gcc/gcc-$1/gcc-$1.tar.bz2" \
        "$GNU_BASE_URL/gcc/gcc-$1/gcc-$1.tar.gz"
}
export -f download_gcc_package

download_gnu_package() {
    command_timer.py -L -i2 -m "downloading $1-$2" -d1 -f "2>&$log_fd" \
    -- download_file.sh \
        --key "$GNU_KEYRING_URL" \
        --signature '%.sig' \
        "$PGT_DOWNLOADS/by-package/" \
        "$GNU_BASE_URL/$1/$1-$2.tar.xz" \
        "$GNU_BASE_URL/$1/$1-$2.tar.bz2" \
        "$GNU_BASE_URL/$1/$1-$2.tar.gz"
}
export -f download_gnu_package

download_package() {
    local name="$1"; shift
    command_timer.py -L -i2 -m "downloading $name" -d1 -f "2>&$log_fd" \
    -- download_file.sh "$PGT_DOWNLOADS/by-package/" "$@"
}
export -f download_package

################################################################################

function_exists() {
    [ "$(type -t "$1")" == function ]
}
export -f function_exists

check_for_functions() {
    local name
    for name in "$@"; do
        function_exists "$name" || fatal "function '$name' doesn't exist"
    done
}
export -f check_for_functions

################################################################################

# sort a series of paths (can be nested at various depths, different parents,
# etc.) by their prefixed index number (e.g. 100-*, 110-*, 250-*, etc.)
sort_paths_by_index() {
    { local p; for p in "$@"; do echo "$p"; done; } \
    | sed -r 's|^(.*/)?([0-9]+)|\2\t\1\2|' \
    | sort -h | sed -r $'s|^([0-9]+)[ \t]+||'
}
export -f sort_paths_by_index

batch_apply_patches() { # patch...
    local path
    #for path in $(sort_paths_by_index $({
    #    local patch
    #    for patch in "$@"; do
    #        echo $(find_in_paths "$PGT_SUPPORT" "patches/$patch")
    #    done
    #})); do
    for path in $(sort_paths_by_index "$@"); do
        patch -tNp$(guess_patch_depth "$path") -i "$path"
    done
}
export -f batch_apply_patches

get_patch_dest_files() { # patch
    # given a patch file, extract all destination paths (rhs) that don't appear
    # to be deletions (1970 and/or /dev/null); here is a rough explanation of
    # our sed script, since (essentially) no one uses sed like this anymore:
    # /^--- /                       look for a left-hand-side line
    # {                             (begin group: lhs)
    #   /(\b1970\b|\/dev\/null\b)/  match "1970" or "/dev/null" in the lhs
    #   !                           invert the match
    #   {                           (begin group: no "1970" or "/dev/null")
    #     n;                        get the next line (rhs)
    #     s/\+\+\+ +([^ \t]+).*/\1/ extract the path from the rhs
    #     p;                        print result
    #   };                          (end group: no "1970" ...)
    # }                             (end group: lhs)
    sed -rn '
    /^--- /{
        /(\b1970\b|\/dev\/null\b)/!{
            n; s/\+\+\+ +([^ 	]+).*/\1/p;
        };
    };
    ' "$1"
}
export -f get_patch_dest_files

guess_patch_depth() { # path
    local dest dest2 n
    # first, try to see if any of the files we're patching exist already
    for dest in $(get_patch_dest_files "$1"); do
        n=0
        while true; do
            # if the file exists, then we've found a match
            if [ -f "${2:-.}/$dest" ]; then
                echo $n
                return 0
            fi
            # if this has no slash in it, just continue
            [[ "$dest" == */* ]] || break
            # cut off the first path component, increment our depth
            dest="${dest#*/}"
            n=$((n+1))
        done
    done
    # nope, all new files apparently; now try to see if any of the parent
    # directories of any of the files we're creating exist
    for dest in $(get_patch_dest_files "$1"); do
        n=0
        # because we're cutting up the path at least once, iterate until there
        # aren't any slashes left in our path
        while [[ "$dest" == */* ]]; do
            # now walk this path chunk backwards, removing the final component
            dest2="${dest%/*}"
            while true; do
                # if the directory exists, then we've found a match
                if [ -d "${2:-.}/$dest2" ]; then
                    echo $n
                    return 0
                fi
                # otherwise, if we have no more slashes, continue
                [[ "$dest2" == */* ]] || break
                # remove the final component from our inner path
                dest2="${dest2%/*}"
            done
            # remove the leading component from our outer path and increment our
            # depth counter before moving on
            dest="${dest#*/}"
            n=$((n+1))
        done
    done
    # we failed to match anything, fail, but print 0 in case the return code
    # doesn't get picked up on as an error
    echo 0
    return 1
}
export -f guess_patch_depth

################################################################################

_is_hex_string() { # <stdin>
    grep -E '^[0-9A-Fa-f]+$' >/dev/null 2>&1
}
export -f _is_hex_string

_to_lower() { # <stdin>
    tr '[[:upper:]]' '[[:lower:]]'
}
export -f _to_lower

_to_upper() { # <stdin>
    tr '[[:lower:]]' '[[:upper:]]'
}
export -f _to_upper

################################################################################

_python_hash() { # algorithm, path
    which python3 >/dev/null 2>&1 \
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
export -f _python_hash

_strip_hash() { # (reads from stdin)
    sed -nr $'s/^([A-Fa-f0-9]+)[ \t].*$/\\1/p'
}
export -f _strip_hash

_generic_shasum() { # algorithm-as-number, path
    if which sha${1:-1}sum >/dev/null 2>&1; then
        command sha${1:-1}sum "${2:--}" | _strip_hash
    elif which shasum >/dev/null 2>&1; then
        shasum -a ${1:-1} "${2:--}" | _strip_hash
    else
        _python_hash sha${1:-1} "$1"
    fi
}
export -f _generic_shasum

_detect_hash_algorithm() { # checksum
    _is_hex_string <<<"$1" \
        || fatal "invalid checksum format: $1"
    case "${#1}" in
    32)  echo md5    ;;
    40)  echo sha1   ;;
    56)  echo sha224 ;;
    64)  echo sha256 ;;
    96)  echo sha384 ;;
    128) echo sha512 ;;
    *)   fatal "unknown checksum type: $1" ;;
    esac
}
export -f _detect_hash_algorithm

md5sum() { # path
    if which md5sum >/dev/null 2>&1; then
        command md5sum "${1:--}" | _strip_hash
    elif which md5 >/dev/null 2>&1; then
        md5 -q "${1:--}"
    else
        _python_hash md5 "$1"
    fi
}
export -f md5sum

sha1sum()   {
    _generic_shasum 1 "$1"
}
export -f sha1sum

sha224sum() {
    _generic_shasum 224 "$1"
}
export -f sha224sum

sha256sum() {
    _generic_shasum 256 "$1"
}
export -f sha256sum

sha384sum() {
    _generic_shasum 384 "$1"
}
export -f sha384sum

sha512sum() {
    _generic_shasum 512 "$1"
}
export -f sha512sum

check_file_hash() { # algorithm, path, checksum
    [ "$1" == none -o "$(_to_lower <<<"$3")" == "$(${1}sum "$2" | _to_lower)" ]
}
export -f check_file_hash

## ARGUMENT PARSING ############################################################

split_args_by_verb() { # verb [verb ...] -- [arg [arg ...]]
    # collect all our verbs
    local -a verbs=()
    while (( $# )); do
        case "$1" in
        --) shift; break ;;
        -*|'') fatal "invalid verb format: '$1'" ;;
        *) verbs+=("$1") ;;
        esac
        shift
    done
    # find all of the arguments occuring prior to a verb
    while (( $# )); do
        # is this a verb we know of?
        [[ " ${verbs[*]} " =~ " $1 " ]] \
            && { printf -- '-- '; break; } \
            || { printf -- '%q ' "$1"; }
        shift
    done
    # dump out the remaining arguments (including the verb)
    while (( $# )); do
        printf -- '%q ' "$1"; shift
    done
    printf '\n'
}

## GENERAL SETUP ###############################################################

# these are programs/packages we -always- use, so check for them early on to
# prevent anything else from dying down the line somewhere
check_for_gnu_prog \
    'cat --version' '\<gnu coreutils\>' \
        --darwin=coreutils:libexec/gnubin
check_for_gnu_prog \
    'which --version' '\<gnu which\>' \
        --darwin=gnu-which:libexec/gnubin
check_for_gnu_prog \
    'sed --version' '\<gnu sed\>' \
        --darwin=gnu-sed:libexec/gnubin
check_for_gnu_prog \
    'find --version' '\<gnu findutils\>' \
        --darwin=findutils:libexec/gnubin
check_for_gnu_prog \
    'getopt --version' '\<util-linux\>' \
        --darwin=gnu-getopt:bin
check_for_gnu_prog \
    'grep --version' '\<gnu grep\>' \
        --darwin=grep:libexec/gnubin
check_for_gnu_prog \
    'libtoolize --version' '\<gnu libtool\>' \
        --darwin=libtool:libexec/gnubin
check_for_gnu_prog \
    'm4 --version' '\<gnu m4\>' \
        --darwin=m4:bin
check_for_gnu_prog \
    'make --version' '\<gnu make [456789]\.' \
        --darwin=make:libexec/gnubin

# these programs -may- get used, but we don't want to die on them unless they
# are actually used, so just wrap them for the time being
eval $(wrap_program_with_check \
        curl 'curl --version' \
            --darwin=curl:bin \
            --debian=curl \
            --ubuntu=curl \
            --fedora=curl \
            --alpine=curl)
eval $(wrap_program_with_check \
        gpg 'gpg --version' \
            --darwin=gpg2:bin \
            --debian=gpg \
            --ubuntu=gpg \
            --fedora=gnupg2 \
            --alpine=gpg)

_get_pgt_paths() {
    local p
    for p in $(_locate_files PGT_DATA "scripts"); do
        printf '%s:' "$p"
    done
}
export PATH=$(_get_pgt_paths)$PATH
unset _get_pgt_paths

