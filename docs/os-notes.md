## OS-Specific Notes

This document provides some examples of how to install the required dependencies
on a variety of host OSes. If you find any errors or want to contribute
additional sections, please submit bugreports and/or pull requests. Any help
here is appreciated!

### macOS

You will first need to acquire [Homebrew][homebrew] in order to install the
required dependencies. Homebrew should prompt you to install the Apple Developer
Tools, but if it does not, run the following command:

```bash
xcode-select --install
```

Once Homebrew is set up, proceed with the following:

```bash
brew install <FIXME>
```

### Linux

The main things required for running under Linux are:

 - a development toolchain with C++ support (almost assuredly GCC)
 - Texinfo (not always necessary, but some packages need it)

The following sections provide examples of installing these dependencies on
several common distros.

#### Ubuntu/Debian

Ubuntu and Debian users should run the following commands to install the
required development tools (and other requirements) from the APT repos:

```bash
apt install -y bash build-essential cmake curl g++ gettext git libglib2.0-dev \
               libtool rsync texinfo
```

#### Fedora

Fedora users should run the following commands to install the required
development tools from their repos:

```bash
dnf groupinstall -y "Development Tools"
dnf groupinstall -y "Development Libraries"
dnf install -y gcc-c++ texinfo libtool perl-open perl-FindBin
```


[homebrew]:     https://brew.sh

