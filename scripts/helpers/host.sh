#!/this/should/be/included/via/bash

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

darwin_get_cpu_cores() {
    if which nproc >/dev/null 2>&1; then
        nproc || echo 1
    elif which sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null || echo 1
    else
        echo 1
    fi
}

# [BEGIN STUPID HACK]
# XXX:  Old code is not aware of arm64/aarch64! While some things can be forced
#       to "work" by setting --host/--build to "arm-apple-darwin", this doesn't
#       work for a few (important, unfortuantely) things, such as GMP. Our very
#       "clever" solution is to force everything to cross-compile for x86_64 on
#       macOS; while we -could- conceivably split the builds into "modern,
#       supports arm64" and "old, doesn't", right now that is a lot of extra
#       work that I'm not sure I have a clean way of doing other than on a
#       per-package basis, so we just are building everything for x86_64. This
#       will probably stop being a viable option in the future (if the
#       transition from PPC->x86 is any indication, Apple will keep the
#       translation layer for a while and eventualy drop it), and at that point,
#       we may just have to say "certain toolchains don't work under macOS", but
#       for now... this clever (read: hacky) solution works fine enough.
# XXX:  This is somewhat hacky! Clang/LLVM, in their INFINITE WISDOM, has made
#       the -Wimplicit-function-declaration ALWAYS ERROR, unless you explicitly
#       disable this behavior via -Wno-error=X (note that -Wno-error DOES NOT
#       FIX THIS!!). Unsurprisingly, this SERIOUSLY BREAKS OLD CODE, because
#       this behavior is unprecedented (if technically correct) and code from
#       mid-2000s does this a LOT. We NEED this to handle modern macOS in any
#       capacity (at least using LLVM, in theory we could use GCC?).
# XXX:  I am VERY WELL AWARE that putting these flags as part of the CC/CXX/LD
#       enivronment variables is -strictly- incorrect and abusing the system;
#       however, some things don't correctly propagate all flags through into
#       their subpackages (*cough*GCC-4.4.7*cough*), so we pretty much are
#       required to do this this way, or things are impossible to get to
#       compile; I had experimented with manually configuring subpackages, but
#       that becomes an awful rabbit hole quickly, and this Just Works(tm).
darwin_stupid_aarch64_hack() {
    declare -a darwin_cross_flags=( \
        -arch x86_64
    )
    declare -a darwin_clang_flags=( \
        "${darwin_cross_flags[@]}"
        -Wno-unused-command-line-argument
    )
    declare -a darwin_cc_flags=( \
        "${darwin_clang_flags[@]}"
        -Wno-error=implicit-function-declaration
    )
    # required compiler flags for making LLVM not break old code
    host_cc+=" ${darwin_cc_flags[*]}"
    host_cpp+=" ${darwin_cc_flags[*]}"
    host_cxx+=" ${darwin_clang_flags[*]}"
    host_ld+=" ${darwin_cross_flags[*]}"
    # define fixed --host/--build values to configure to prevent detection;
    # autoconf complains if you provide them in an order that isn't --build,
    # --host, --target? so we're doing that, I guess
    configure_flags+=(
        --build=x86_64-apple-darwin
        --host=x86_64-apple-darwin
    )
    cmake_flags+=(
        -DCMAKE_C_FLAGS="${darwin_cc_flags[*]}"
        -DCMAKE_CXX_FLAGS="${darwin_clang_flags[*]}"
        -DCMAKE_EXE_LINKER_FLAGS="${darwin_cross_flags[*]}"
    )
}
# [END STUPID HACK]

darwin_fixup() {
    darwin_stupid_dsym_hack
}

# [BEGIN STUPID HACK]
# XXX: This is ALSO somewhat hacky! Because macOS uses Mach-O binaries, and
#      because for whatever reason, they don't pack around all their debug
#      information inline (they're separated out as *.dSYM **FOLDERS**), any
#      time old code tries to check whether the -g flag works, things explode
#      because old autotools handled cleanup with 'rm -f' not 'rm -rf' and the
#      debug symbols can't be removed. We can "fix" this by just dumping all of
#      the *.dSYM folders out into a random directory and never looking at them
#      again. THANKS LLVM, YOU'RE SO AWESOME.
darwin_stupid_dsym_hack() {
    dsym_trash="$tmp_dir/dsym_trash"
    # make our stupid symbol directory so we can never look at it 
    mkdir -p "$dsym_trash"
    # fix our cflags to include the aforementioned ~~aWeSoMe fIx~~
    declare -a darwin_dsym_flags=( \
        -dsym-dir "$dsym_trash"
    )
    host_cc+=" ${darwin_dsym_flags[@]}"
    host_cxx+=" ${darwin_dsym_flags[@]}"
}
# [END STUPID HACK]

################################################################################

generic_get_cpu_cores() {
    if which nproc >/dev/null 2>&1; then
        nproc || echo 1
    else
        echo 1
    fi
}

################################################################################

# modify environment for specific host build environments
export host_platform="$(uname -s)"
case "${host_platform}" in
Darwin)
    make_flags+=("-j$(darwin_get_cpu_cores)")
    host_cc="${host_cc:-${CC:-gcc}}"
    host_cpp="${host_cpp:-${CPP:-gcc -E}}"
    host_cxx="${host_cxx:-${CXX:-g++}}"
    host_ld="${host_ld:-${LD:-ld}}"
    darwin_stupid_aarch64_hack
    darwin_fixup
    ;;
*)
    make_flags+=("-j$(generic_get_cpu_cores)")
    ;;
esac

