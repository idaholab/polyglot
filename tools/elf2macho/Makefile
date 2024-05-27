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

LDLIBS += -lelf

progs := $(cachedir)/elf2macho

all: $(progs)
clean: $(progs)
install: $(progs)

$(cachedir)/elf2macho: $(cachedir)/elf2macho.o
$(cachedir)/elf2macho.o: elf2macho.cpp macho.hpp

