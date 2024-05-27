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


include host-tool.mk

progs := $(cachedir)/modify_elf

all: $(progs)
clean: $(progs)
install: $(progs)

$(cachedir)/modify_elf: $(cachedir)/modify_elf.o
$(cachedir)/modify_elf.o: modify_elf.cpp elf_lookup.hpp

