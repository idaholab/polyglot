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


need_goals     := no

pgt_pathsep    := :
pgt_pathset     = $(subst $(pgt_pathsep), ,$(PGT_$1))
expand_paths    = $(wildcard $(foreach b,$1,$(foreach e,$2,$b/$e)))
pgt_paths       = $(call expand_paths,$(call pgt_pathset,$1),$2)

ifeq ($V,1)
Q              :=
MSG1           :=
MSG2           :=
else
Q              := @
MSG1            = printf '  %s\n' '$1'
MSG2            = printf '  %-8s %s\n' '$1' '$2'
endif

this_makefile   = $(realpath $(firstword $(MAKEFILE_LIST)))

ifeq ($(MAKELEVEL),0) ##########################################################

# make sure our required variables are defined immediately
$(if $(target),,$(error `target' variable must be specified))
$(if $(prefix),,$(error `prefix' variable must be specified))
$(if $(cachedir),,$(error `cachedir' variable must be specified))

# figure out our include paths, but don't propagate them via recursion
unexport include_dirs = $(call expand_paths,$(lib_dirs),makefiles)

.PHONY: all $(MAKECMDGOALS)
all $(filter-out all,$(MAKECMDGOALS)): | _recurse ;

.PHONY: _recurse
_recurse:
	@mkdir -p $(prefix)
	$(MAKE) --no-builtin-rules --no-print-directory \
        -f$(this_makefile) $(addprefix -I,$(include_dirs)) \
        $(MFLAGS) $(MAKECMDGOALS)

else ###########################################################################

# specify this in case our inclusion defines any rules
.DEFAULT_GOAL  := all

header_exts    := h hpp

CC             := $(target)-gcc
AR             := $(target)-ar
CPPFLAGS        =
CFLAGS          = -g
ASMFLAGS        =
DEPFLAGS        = -MD -MP -MF $(depcache)/$*.d

ALL_C_FLAGS     = $(CPPFLAGS) $(DEPFLAGS) $(CFLAGS)
ALL_ASM_FLAGS   = $(CPPFLAGS) $(DEPFLAGS) $(ASMFLAGS)

RSYNC          ?= rsync
RSYNCFLAGS      = -Lcrd $(header_exts:%=--include="*.%") \
                  --include="*/" --exclude='*'

# also these, for the same reason
gcclib         := $(prefix)/lib/gcc/*/*
gcclibexec     := $(prefix)/libexec/gcc/*/*
targetdir      := $(prefix)/target
includedir     := $(targetdir)/include
libdir         := $(targetdir)/lib
objcache       := $(cachedir)/objects
depcache       := $(objcache)/deps
linkcache      := $(objcache)/links
headercache    := $(cachedir)/headers
fixheadercache := $(cachedir)/fixed-headers

#-------------------------------------------------------------------------------

_target_bits   := $(subst -, ,$(target))
target_arch     = $(let v,$(word 1,$(_target_bits)),$(if $v,$v,unknown))
target_os       = $(let v,$(word 2,$(_target_bits)),$(if $v,$v,none))
target_binfmt   = $(let v,$(word 3,$(_target_bits)),$(if $v,$v,binary))
target_abi      = $(let v,$(word 4,$(_target_bits)),$(if $v,$v,default))

#-------------------------------------------------------------------------------

$(if $(wildcard FORCE),$(error FORCE file exists -- rules -WILL- break))

FORCE:

define do_rsync
@mkdir -p $3
@$(call MSG2,RSYNC,$(strip $1))
$Qrsync $(if $4,$4,$(RSYNCFLAGS)) $2 $3
endef

$(headercache)/%:
	$(if $^,,$(error no source(s) for '<headercache>/$*'))
	$(call do_rsync, <headercache>/$*, \
        $(patsubst %,%/,$(filter-out FORCE,$^)), $@/)

#-------------------------------------------------------------------------------

# include the target-specific makefile bits
include $(target).mk

#-------------------------------------------------------------------------------

forcing_clean := no

# we only check for newness of makefiles if we are doing something other than
# (or in addition to) installing headers; this way, the headers-install goal
# doesn't have the side effect of cleaning things
ifneq ($(filter-out $(MAKECMDGOALS),headers-install),)

SPECFILE_LIST = $(shell $(CC) -v 2>&1 | sed -rn 's/^Reading specs from //p')

get_oldest_cachedir_file = { \
        find "$(cachedir)" -type f -exec stat -c '%Y' {} \+ 2>/dev/null \
            | sort -u; \
        echo 9999999999; \
    } | head -n1
get_newest_depfile = { \
        stat -c '%Y' $(MAKEFILE_LIST) $(SPECFILE_LIST) 2>/dev/null \
            | sort -ru; \
        echo 0; \
    } | head -n1
any_in_cachedir_out_of_date = $(shell \
    test $$($(get_oldest_cachedir_file)) -le $$($(get_newest_depfile)) \
        && echo yes \
        || true)

ifeq ($(any_in_cachedir_out_of_date),yes)

$(warning Makefiles or specs newer than files in cachedir; forcing clean...)

forcing_clean := yes

.PHONY: all $(MAKECMDGOALS)
all $(filter-out all,$(MAKECMDGOALS)): _clean _recurse ;

.PHONY: _recurse
_recurse:
	@$(MAKE) -f$(this_makefile) $(MFLAGS) $(MAKECMDGOALS)

endif # any files in cachedir out of date
endif # goals other than headers-install

ifneq ($(forcing_clean),yes) ###################################################

need_goals     := yes

object_paths    = $(patsubst %,$(objcache)/%.o,$(basename $1))
dep_paths       = $(patsubst %,$(depcache)/%.d,$(basename $1))
link_paths      = $(patsubst %,$(linkcache)/%.o,$(subst /,_,$(basename $1)))
undo_link_paths = $(call object_paths,$(foreach p,$1,$(foreach s,\
                        $(sources),$(if $(filter $(notdir \
                        $(call link_paths,$s)),$p),$s))))
reverse         = $(let f r,$1,$(if $r,$(call reverse,$r) )$f)

#-------------------------------------------------------------------------------

BASH           ?= $(shell which bash)
MKHEADERS      ?= $(firstword $(wildcard $(gcclibexec)/install-tools/mkheaders))
include_fixed  := $(firstword $(wildcard $(gcclib)/include-fixed))

#-------------------------------------------------------------------------------

vpath %.c $(call reverse,$(source_paths))
vpath %.S $(call reverse,$(source_paths))

#-------------------------------------------------------------------------------

.PHONY: all
all:

.PHONY: clean
clean: _clean

.PHONY: install
install:

#-------------------------------------------------------------------------------

$(headercache):
	@$(call MSG2,MKDIR,<headercache>)
	$Qmkdir -p $@

#-------------------------------------------------------------------------------

.PHONY: headers-install
headers-install: headers-copy headers-fix

.PHONY: headers-copy
headers-copy: $(header_targets)
	@mkdir -p $(includedir)
	$(call do_rsync, include/, $(headercache)/, $(includedir)/, -Ldru)

.PHONY: headers-fix
ifneq ($(MKHEADERS),)
$(if $(include_fixed),,$(error mkheaders found, but didn't find \
    lib/gcc/.../include-fixed/ directory))
# run GCC's mkheaders script (if found), but utilize our patched version in
# order to write the new headers to a temporary directory, then -selectively-
# sync the temp dir with the real one (based on checksum) so we avoid touching
# as many files as possible (in order to prevent other things from rebuilding
# if at all possible)
headers-fix:
	@mkdir -p $(fixheadercache)
	@$(call MSG1,MKHEADERS)
	$QCONFIG_SHELL=$(BASH) incdir=$(fixheadercache) $(MKHEADERS)
	@$(call MSG2,RSYNC,<include-fixed>)
	$Qrsync -cldr --delete $(fixheadercache)/ $(include_fixed)/
else
headers-fix: ;
endif

#-------------------------------------------------------------------------------

$(cachedir)/%.a:
	@mkdir -p $(@D)
	@$(call MSG2,AR,$*.a (updating $(words $?) objects))
	$Q$(AR) cru $@ $?

$(libdir)/%.a: $(cachedir)/%.a
	@mkdir -p $(@D)
	@$(call MSG2,CP,$*.a)
	$Qcp -f $< $@

$(objcache)/%.o: %.c
	@mkdir -p $(@D) $(dir $(depcache)/$*.d)
	@$(call MSG2,CC,$*.c)
	$Q$(CC) $(ALL_C_FLAGS) -c -o $@ $<

$(objcache)/%.o: %.S
	@mkdir -p $(@D) $(dir $(depcache)/$*.d)
	@$(call MSG2,CC,$*.S)
	$Q$(CC) $(ALL_ASM_FLAGS) -c -o $@ $<

$(linkcache)/%:
	@mkdir -p $(@D)
	$Qln -s $(firstword $(call undo_link_paths,$*)) $@

#-------------------------------------------------------------------------------

include $(wildcard $(call dep_paths,$(sources)))

endif # forcing_clean ##########################################################

.PHONY: _clean
_clean:
	@$(call MSG2,RM,<cachedir>)
	$Qrm -rf $(cachedir)

endif # MAKELEVEL ##############################################################
