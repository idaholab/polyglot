![Polyglot](docs/images/polyglot-logo.png#gh-light-mode-only)
![Polyglot](docs/images/polyglot-logo-dark.png#gh-dark-mode-only)

## What is Polyglot?

Polyglot is a framework for building tools to run on a variety of targets, with
a specific focus on embedded/ICS and ancient devices. The goal is to provide a
GCC- and POSIX-like build environment for these targets to allow for development
without needing to track down arcane toolchains or understand specific details
of a given target. While Polyglot was original developed with the intent of
quickly porting forensic tools to embedded platforms, it definitely could be
useful in a wide variety of use cases.

Polyglot has two main components:

- a tool for building/maintaining supported toolchains
- a custom, minimal C library designed for portability and simplicity

The general workflow is to use the former to build the toolchain for a target,
then use that toolchain (which includes the latter) to compile executables to
run on that target.

## Getting Started

This document provides some basic documentation for interacting with Polyglot
from a _user_ perspective (i.e. you're building tools using Polyglot, not
developing support for targets or modifying the C library); for the _developer_
equivalent, please see [the developer notes][devnotes].

### Cloning this Repository

This repository requires submodules to function properly; this means that after
you have cloned the repository, you will need to pull all submodules using the
following command:

```bash
git submodule update --init
```

Release tarballs include all required submodules already in place.

### Dependencies

Overall, Polyglot doesn't have a lot of dependencies; primarily, you will need a
development environment for your host, although on some hosts (here's looking at
you, macOS), you may need additional tools. See the [OS specific notes][osnotes]
for example dependency installation commands for some hosts (although YMMV).

### Installation

Polyglot is able to be used in place, in which case it will store all files
locally (within `.state` and `.cache` directories within the repository root).

> At some point in the future, it will also be able to be installed using an
> included script. The main script (`polyglot`) already supports finding files
> in standard XDG locations, so it's mostly just copying things to the right
> spots.

### Basic Usage

This section describes some simple usage examples; for further information, see
the help pages for the various Polyglot subcommands.

#### Listing Targets

To see the targets available to the Polyglot instance, use the `list` verb; for
example:

```bash
polyglot list
```

This verb supports basic, Bash-style globbing on target names; the results can
also be filtered using `--installed`/`--uninstalled`.

#### Building/Cleaning Targets

To build a toolchain, use the `build` verb; for example:

```bash
polyglot build gcc-i386-linux-elf
```

To uninstall/clean a toolchain, use the `clean` verb; for example:

```bash
polyglot clean gcc-i386-linux-elf
```

Both of these verbs support basic, Bash-style globbing on target names.

#### Using Targets

To "activate" target toolchains (i.e. make them available in your current
environment), use the `activate` verb; for example:

```bash
eval $(polyglot activate gcc-i386-linux-elf)
```

Once activated, they can be deactivated using the `deactivate` shell function
that the activation installs:

```bash
deactivate
```

This verb support basic, Bash-style globbing on target names, and multiple
targets may be specified at once.

Once a target is activated, target-specific tools are available in the
environment; for example:

```bash
cat > test.c <<EOF
#include <stdio.h>

int main(int argc, char **argv) {
    printf("Hello, world!\n");
    return 0;
}
EOF
i386-linux-elf-gcc -o test test.c
qemu-i386-static test
```

## Additional Information

### Rationale

In the strictest sense, we don't.

However, Polyglot exists to address several limitations in doing what it does
manually. Take for example the following examples of challenges in building
executables for the sorts of devices Polyglot targets (or may someday):

 - Linux has run on a _wide variety_ of architectures. Some of these are common,
   others... less so. While other tools (such as Crosstool-NG) support many of
   them, many also are too obscure (or ancient) for these sorts of tools.
 - Some architectures have modern GCC support. Some have GCC support prior to a
   specific version. Some have support via forked repos or sets of patches. A
   few only have support via sets of patches hosted on university websites that
   are only accessible via the Wayback Machine. Figuring each of these out adds
   to the pain of trying to build code for these targets.
 - Most modern C libraries are not designed to be statically linked. Because one
   doesn't know what versions of libraries are on embedded systems (and in some
   cases, the manufacturer has actually stripped shared libraries of all
   relevant info, past the normal symbol stripping one might expect), dynamic
   linking is out. The Polyglot C library helps to solve this by being designed
   around being statically linked.
 - As a specific example, modern LLVM/Clang can't build 32-bit PowerPC Mach-O
   binaries... which means that the only way to build executables for old
   versions of MacOSX is to install XCode on actual installs of those versions
   of MacOSX. Which is exactly how the few projects that maintain modern
   browsers, etc. for these targets build code.

So while one can generally do what Polyglot does using other tools, Polyglot
serves to fill that gap and provide a more straightforward, direct approach to
the problem.

It is worth noting that there are some caveats to using Polyglot:

 - There are a lot of interesting challenges in building code for many
   architectures and/or targets; while Polyglot tries to address these, I'm sure
   there are plenty that I am unaware of that need tackling, especially as
   support for more embedded targets are added.
 - While other C libraries have been around a long time and had a lot of eyes on
   them, `polyglot-libc` is very young and needs a lot of TLC to be at the point
   any of the others are. This will (hopefully) come with time and interest from
   the community.
 - `polyglot-libc` is minimal (by design), which means that many of the things a
   modern developer might expect (such as threads) don't exist. Some of that is
   intentional (and likely to staythat way... such as threading), but some is
   just an indication of needing further development.
 - _Bugs._ I've tested a lot of things. That being said, there isn't currently a
   testing framework, and bugs have a nasty habit of creeping in over time. Not
   to mention corner cases that I haven't stumbled upon. This will also
   (hopefully) come with interest and use from the community.

While Polyglot is, in many ways, still very much a work in progress, it does
currently work for a subset of use cases, and begins to tackle the problems it
aims at. Hopefully it can be a solution for some problems as it currently
exists, but also can continue to be developed into a more generally useful tool.

### About the Logo

The logo is a Northern mockingbird, which, funnily enough, has the scientific
name _Mimus polyglottos_. I stumbled across this fact from an unrelated source,
and it seemed too much of a coincidence to pass up.

[This specific picture][mockingbird] is from Wikimedia Commons, and is in the
public domain. It has been modified/vectorized specifically for this
repository's logo using Adobe Photoshop and Adobe Illustrator.

The font used is a variant of [Iosevka][iosevka].

## License

This repository is licensed as [GPL v3][license]. For additional information
regarding funding, etc. see the [notice][notice] file in this repository.


[devnotes]:     docs/dev-notes.md
[osnotes]:      docs/os-notes.md
[license]:      LICENSE_GPL3.txt
[notice]:       NOTICE.txt
[mockingbird]:  https://commons.wikimedia.org/wiki/File:Mimus_polyglottos1.jpg
[iosevka]:      https://typeof.net/Iosevka/

