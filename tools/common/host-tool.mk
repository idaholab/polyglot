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


.DEFAULT_GOAL  := all

################################################################################

CFLAGS         +=
CPPFLAGS       +=
CXXFLAGS       += -std=gnu++2b
LDFLAGS        +=
LDLIBS         +=

cachedir       ?= .
src            ?= $(dir $(realpath $(word 1,$(MAKEFILE_LIST))))
inc            ?= $(src)
bin_prefix     ?=
prefix         ?=

################################################################################

_cc1           := $(if $(prefix),$(wildcard $(prefix)/libexec/gcc/*/*/cc1))
libexec        := $(if $(_cc1),$(dir $(_cc1)))

################################################################################

vpath %.c   $(src)
vpath %.cpp $(src)
vpath %.h   $(inc)
vpath %.hpp $(inc)

.PHONY: all
all:

.PHONY: clean
clean: $(cachedir)/*.o
	rm -f $^

.PHONY: install
ifeq ($(prefix),)
install:
	$(error no 'prefix' variable defined, cannot install)
else
install:
	mkdir -p $(prefix)/bin
	for path in $^; do \
        prog=$$(basename $$path); \
        tgt_prog=$(bin_prefix)$$prog; \
        [ "$(prefix)/bin/$$tgt_prog" -nt "$$path" ] \
            || { cp -f $$path $(prefix)/bin/$$tgt_prog; \
                 chmod +x $(prefix)/bin/$$tgt_prog; }; \
        $(if $(libexec),,true || )[ -e "$(libexec)/$$prog" ] \
            || ln -s ../../../../bin/$$tgt_prog $(libexec)/$$prog; \
    done
endif

################################################################################

$(cachedir):
	mkdir -p $(cachedir)

$(cachedir)/%: | $(cachedir)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(cachedir)/%.o: %.cpp | $(cachedir)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

