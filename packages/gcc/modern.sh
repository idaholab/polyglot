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
export gcc_target="${options[gcc-target]:-$target}"
export sysroot="$target_prefix"

declare -a target_bits=(${target//-/ })
export target_arch=${options[target-arch]:-${target_bits[0]:-unknown}}
export target_os=${options[target-os]:-${target_bits[1]:-none}}
export target_binfmt=${options[target-binfmt]:-${target_bits[2]:-binary}}
export target_abi=${options[target-abi]:-${target_bits[3]:-default}}

export -a specs_files=( \
    "specs"
    "polyglot/common.specs"
    "polyglot/target.specs"
    "polyglot/$target.specs"
    "${extra_specs_files[@]}"
)

################################################################################

gcc_options()
{
    local opt with
    while (( $# )); do
        if [[ "$1" == *:* ]]; then
            opt="${1%%:*}"
            with="${1#*:}"
        else
            opt="$1"
            with="$1"
        fi
        [ "${options[gcc-$opt]}" ] && \
            gcc_configure_options+=( "--with-$with=${options[gcc-$opt]}" )
        shift
    done
}

################################################################################

check_for_functions \
    download_gcc download_gmp download_isl download_mpc download_mpfr

################################################################################

do_extract()
{
    set -exo pipefail
    extract_archive.sh "${files[gcc]}" "$src_dir"
    extract_archive.sh "${files[gmp]}" "$src_dir/gmp"
    extract_archive.sh "${files[isl]}" "$src_dir/isl"
    extract_archive.sh "${files[mpc]}" "$src_dir/mpc"
    extract_archive.sh "${files[mpfr]}" "$src_dir/mpfr"
}
export -f do_extract

do_patch()
{
    set -exo pipefail
    cd "$src_dir"
    batch_apply_patches $(find_package_files \
            "patches/*.patch" \
            "patches/$version/*.patch")
}
export -f do_patch

do_configure()
{
    set -exo pipefail
    cd "$build_dir"
    "$src_dir/configure" \
        "${configure_flags[@]}" \
        "${gcc_configure_options[@]}" \
        --target=$gcc_target \
        --program-prefix=$target- \
        --prefix="$prefix" \
            `#  where we are installing our toolchain` \
        --with-local-prefix="$sysroot/local" \
            `#  this -doesn't- get changed by --with-sysroot (which, for \
             #  non-cross-compiling setups, makes sense), but we never want to \
             #  accidentally look at the host includes/libraries for anything, \
             #  so just move it inside our tree` \
        --with-sysroot="$sysroot" \
            `#  move our sysroot to within our tree, which simplifies things, \
             #  but does bind us to a single target; this is a change from how \
             #  Polyglot was historically set up, but removes a lot of the \
             #  other hoops we had to jump through previously` \
        --with-native-system-header-dir="/include" \
            `#  this is relative to our sysroot (see --with-sysroot above), \
             #  and specifies where within that tree the system includes live; \
             #  by default it's /usr/include, but we really aren't a proper \
             #  "system root tree", so we just use a simpler path` \
        `#--without-headers` \
            `#  we previously needed this option, but if we install headers \
             #  first, then we don't need this and can build target support \
             #  libraries, so it's (mostly) a win-win (the only cost is that \
             #  we need to have a toolchain per-target, but that's the saner \
             #  way of doing it anyway, so it's still better this way)` \
        --disable-werror \
            `#  don't use -Werror=all for building, we get lots of random crap \
             #  warnings due to LLVM being its nonsensical self on macOS (and
             #  FreeBSD, I guess), so this option is pretty much required` \
        --disable-shared \
            `#  this disables shared versions of the target libraries being \
             #  built (libgcc, libgcov, etc.), but not the host tools` \
        --disable-threads \
            `#  this is only relevant if we end up building C++/ObjC/ObjC++ \
             #  support libraries, but it doesn't lose us anything and it \
             #  saves us in case we ever reenable any of those` \
        --disable-libquadmath \
            `#  libquadmath needs proper wchar_t support, which we don't have \
             #  at all currently (we're missing things like putwc(), etc.)` \
        --disable-bootstrap \
            `#  so this is -supposed- to be the default option post-4.3, but \
             #  in practice, it must be being automatically enabled due to \
             #  something about our configuration/setup, so we have to disable \
             #  it or we get some weird problems with some other options; we \
             #  may be able to remove this at this point, I'm not sure, and if \
             #  we ever do a proper multi-stage build, then we probably -do- \
             #  need to remove it to make that work` \
        --disable-gcov \
            `#  gcov needs <sys/mman.h> presence with basic definitions \
             #  (including MAP_*, PROT_*, etc.), which -right now- we are not \
             #  planning on necessarily including, but if it makes sense, we \
             #  could add those and remove this option` \
        --enable-languages=c \
            `#  currently only C language support works; can we enable C++ \
             #  support at some point too?` \
        --enable-plugin \
            `#  this was here because we were building a plugin to help us do \
             #  some code analysis in order to not need to link everything \
             #  (namely printf() related); we're not doing that kind of \
             #  analysis anymore, but we're still building the plugin (as a \
             #  way for people to hook things in the future if needed), so we \
             #  need this to make that function properly` \
        --enable-multilib \
            `#  yes, this is the "classic" way of handling things, but it \
             #  gives us a bit more flexibility as to what all falls inside a \
             #  single target; namely, this is relevant for biendian arches \
             #  and/or those with optional FPUs` \
        --enable-obsolete \
            `#  some of our targets are considered "obsolete", so regardless \
             #  of what target we're building, we say obsolete is okay since \
             #  it doesn't lose us anything ` \
        --with-newlib \
            `#  this is supposed to remove __eprintf, but in practice, for \
             #  modern GCC, it doesn't do that on some (all?) targets; thus we \
             #  mostly need the patch applied above, but I'm leaving this in \
             #  case it does work somewhere... ` \
        --disable-tm-clone-registry \
        --enable-sjlj-exceptions \
        --disable-libssp \
        --disable-libatomic \
        --disable-libgomp \
        --disable-default-pie \
        "${options[gcc-configure-extra]}"
}
export -f do_configure

do_host_build()
{
    set -exo pipefail
    make -C "$build_dir" "${make_flags[@]}" all-host
}
export -f do_host_build

do_host_install()
{
    local fname prog target_gcc dest_dir
    set -exo pipefail
    make -C "$build_dir" "${make_install_flags[@]}" installdirs install-host

    # walk through the programs that got installed and fix some corner cases
    for fname in $(ls "$prefix_bin"); do
        case "$fname" in
        # unlink any existing '-local-' variants, since they shouldn't be in
        # this directory to begin with
        $target-local-*)
            unlink "$fname"
            ;;
        # if it's a properly-named executable, link a '-local-' variant to
        # handle build systems that refuse to accept our triple as valid
        $target-*)
            prog=${fname:$((${#target}+1)):${#fname}}
            [ -e "$prefix_localbin/$target-local-$prog" ] \
                || ln -s "$prefix_bin/$fname" \
                    "$prefix_localbin/$target-local-$prog"
            ;;
        # if it begins with the GCC target prefix, move it into the
        # target-specific binary directory that we (probably) already have
        $gcc_target-*)
            mkdir -p "$prefix/$gcc_target"
            prog="${fname:$((${#gcc_target}+1)):${#fname}}"
            mv "$prefix_bin/$fname" "$prefix/$gcc_target/bin/$prog"
            ;;
        # otherwise, move it to a special "extra binaries" directory that we
        # create... it isn't to spec, but we need these files moved
        *)
            mkdir -p "$prefix_extrabin"
            mv "$prefix_bin/$fname" "$prefix_extrabin/"
            ;;
        esac
    done

    # "fix" our mkheaders script to allow us to write the fixed headers to a
    # nonstandard location (but still to the default if unspecified)
    sed -i -r 's/^incdir=/[ "$incdir" ] || incdir=/' \
        "$prefix/libexec/gcc/$gcc_target/$version/install-tools/mkheaders"
    # some architectures need additional files copied over that aren't
    # installed automatically (TODO: why is this?) so we copy them manually
    target_gcc="$prefix_bin/$target-gcc"
    dest_dir="$("$target_gcc" -print-file-name=plugin)/include/config"
    case "$target" in
    arc-unknown-elf)
        cp -rn "$src_dir/gcc/config/arc"/* "$dest_dir/arc/" || true
        ;;
    csky-unknown-elf)
        cp -rn "$src_dir/gcc/config/csky"/* "$dest_dir/csky/" || true
        ;;
    xtensa-unknown-elf)
        cp -rn "$src_dir/gcc/config/xtensa"/* "$dest_dir/xtensa/" || true
        cp "$src_dir/include/xtensa-config.h" "$dest_dir/xtensa/" || true
        ;;
    esac
    # some things may need these libraries to be present (in particular,
    # plugin support, I believe), so we install them here rather than
    # building them separately (although we could do that, and it might be
    # a better idea)
    make -C "$build_dir/gmp" "${make_install_flags[@]}" install
    make -C "$build_dir/isl" "${make_install_flags[@]}" install
    make -C "$build_dir/mpc" "${make_install_flags[@]}" install
    make -C "$build_dir/mpfr" "${make_install_flags[@]}" install
}
export -f do_host_install

################################################################################

generate_specs()
{
    local temp_specs_dir="$tmp_dir/specs"
    local specs_dir="$prefix/$gcc_target/lib"
    local specfile specbase spectemp

    # clean up an existing temp specs directory
    rm -rf "$temp_specs_dir"
    mkdir -p "$temp_specs_dir"

    # copy all of our given specs files over, replacing embedded variables
    for specbase in "${specs_files[@]}"; do
        specfile=$(find_package_files "specs/$specbase" | head -n1) \
            || fatal "unable to locate GCC '$specbase' file"
        spectemp="$temp_specs_dir/$specbase"
        mkdir -p "$(dirname "$spectemp")"
        envsubst < "$specfile" > "$spectemp"
    done

    # rsync the temp specs dir to the real one, checking for differences via
    # checksum; if any were changed, clean the target files before continuing
    if (( $(rsync --info copy1,name1 -lcrd "$temp_specs_dir/" "$specs_dir/" \
                | wc -l) > 0 )); then
        log_command 'cleaning target files' -- do_target_clean
    fi
}

do_target_sanity()
{
    set -exo pipefail
    cd "$build_dir"
    echo 'int test_function(void) { return 0; }' \
        | $target-gcc -xc -c -o sanity-check.o -
    test -s sanity-check.o
    # log some information about the sanity check object
    stat sanity-check.o
    file sanity-check.o
    # this is mostly for debugging, but dump a bunch of ELF info to the log
    # too, so that we can do some verification that the resulting binaries
    # look the way we're expecting for the toolchain
    $target-readelf -esVALW sanity-check.o
}
export -f do_target_sanity

do_target_clean()
{
    set -exo pipefail
    # clean the gcc target libraries so they will fully rebuild
    make -C "$build_dir" "${make_flags[@]}" distclean-target
    # remove target-related things:
    # 1) the entire target directory, since it contains artifacts built with the
    #    previous target/specs state that might not get replaced
    # 2) any target-specific stampfiles to force them to rebuild
    # 3) any target-specific build state so all rebuilt packages are clean
    rm -rf "$target_prefix" "$prefix_stamp/target" "$prefix_tmp/target"
}
export -f do_target_clean

do_target_build()
{
    set -exo pipefail
    # force our target build to use proper gcc instead of its internal xgcc,
    # since the latter doesn't respect in-tree specs files and will explode
    make -C "$build_dir" "${make_flags[@]}" all-target \
                CC_FOR_TARGET=$target-gcc CC_FOR_BUILD=$target-gcc
}
export -f do_target_build

do_target_install()
{
    set -exo pipefail
    make -C "$build_dir" "${make_install_flags[@]}" install-target
}
export -f do_target_install

################################################################################

export -a gcc_configure_options=()
gcc_options cpu cpu-32 cpu-64 arch arch-32 arch-64 tune tune-32 tune-64 \
            schedule abi fpu float simd

case "${config:-default}" in
host)
    export -A files=()
    if ! is_source_extracted; then
        files[gcc]="$(download_gcc)"
        files[gmp]="$(download_gmp)"
        files[isl]="$(download_isl)"
        files[mpc]="$(download_mpc)"
        files[mpfr]="$(download_mpfr)"
        log_command 'extracting' -- do_extract
        log_command 'patching' -- do_patch
        mark_source_extracted
    fi
    if needs_configured; then
        log_command 'configuring' -- do_configure
    fi
    log_command 'building host files' -- do_host_build
    log_command 'installing host files' -- do_host_install
    # link this build directory in place of the target one, since we need the
    # context of this host build for the target library builds
    ln -s "$(basename "$tmp_dir")" "$prefix_tmp/$pkg_group/$pkg_basename-specs"
    ln -s "$(basename "$tmp_dir")" "$prefix_tmp/$pkg_group/$pkg_basename-target"
    finalize_package --keep-tmp --keep-src
    ;;

specs)
    generate_specs
    log_command 'checking compiler sanity' -- do_target_sanity
    ;;

target)
    log_command 'building target files' -- do_target_build
    log_command 'installing target files' -- do_target_install
    ;;

*)
    fatal "unknown configuration: ${config:-default}"
    ;;
esac

