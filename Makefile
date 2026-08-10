# ----------------------------------------------------- #
# Makefile for the gladiator game module for Quake II   #
#                                                       #
# Just type "make" to compile the                       #
#  - Ground Zero Game (game.so)                         #
#                                                       #
# Dependencies:                                         #
# - None, but you need a Quake II to play.              #
#   While in theorie every client should work           #
#   Yamagi Quake II ist recommended.                    #
#                                                       #
# Platforms:                                            #
# - FreeBSD                                             #
# - Linux                                               #
# - Mac OS X                                            #
# - OpenBSD                                             #
# - Windows                                             #
# ----------------------------------------------------- #

# On the Windows agent's WSL1 host, `uname -s` reports plain "Linux" -- WSL1
# has no SystemRoot, MSYSTEM, or MINGW_CHOST to signal otherwise -- so with
# no explicit CC= this Makefile would otherwise silently default to a native
# .so build that is never actually used on that host (this exact WSL1
# environment only ever builds gladiator.dll; see CLAUDE.md's two-agent
# table). Auto-select the 32-bit mingw-w64 cross compiler there instead, but
# only when the caller didn't already choose a compiler: $(origin CC) is
# "default" only when nothing (not `make CC=...`, not a CC env var) overrode
# GNU Make's built-in `CC = cc` -- note plain `CC ?= ...` can NOT be used for
# this, since that built-in default already counts as "having a value" and
# silently wins over `?=`.
ifeq ($(origin CC), default)
ifneq (,$(wildcard /mnt/c))
ifneq (,$(WSL_DISTRO_NAME))
CC := i686-w64-mingw32-gcc
endif
endif
endif

# Detect the OS
ifdef SystemRoot
YQ2_OSTYPE ?= Windows
else
YQ2_OSTYPE ?= $(shell uname -s)
endif

# Special case for MinGW and MSYS2 (both produce Windows PE32 DLLs via MinGW gcc)
ifneq (,$(findstring MINGW,$(YQ2_OSTYPE)))
YQ2_OSTYPE := Windows
endif
ifneq (,$(findstring MSYS,$(YQ2_OSTYPE)))
YQ2_OSTYPE := Windows
endif

# Special case for cross-compiling the Windows PE32/PE32+ DLLs directly with
# the mingw-w64 toolchain from a plain Linux/WSL1 shell -- no MSYS2 install,
# MSYSTEM environment, or Windows env vars required:
#   make CC=i686-w64-mingw32-gcc      -> 32 bit gladiator.dll / game.dll
#   make CC=x86_64-w64-mingw32-gcc    -> 64 bit gladiator.dll / game.dll
ifneq (,$(findstring w64-mingw32,$(CC)))
YQ2_OSTYPE := Windows
endif

# Detect the architecture
ifeq ($(YQ2_OSTYPE), Windows)
ifneq (,$(findstring x86_64-w64-mingw32,$(CC)))
YQ2_ARCH ?= x86_64
else ifneq (,$(findstring i686-w64-mingw32,$(CC)))
YQ2_ARCH ?= i386
else ifdef MINGW_CHOST
ifeq ($(MINGW_CHOST), x86_64-w64-mingw32)
YQ2_ARCH ?= x86_64
else # i686-w64-mingw32
YQ2_ARCH ?= i386
endif
else # windows, but MINGW_CHOST not defined and CC isn't a w64-mingw32-* cross compiler
ifdef PROCESSOR_ARCHITEW6432
# 64 bit Windows
YQ2_ARCH ?= $(PROCESSOR_ARCHITEW6432)
else
# 32 bit Windows
YQ2_ARCH ?= $(PROCESSOR_ARCHITECTURE)
endif
endif # windows but MINGW_CHOST not defined and CC isn't a w64-mingw32-* cross compiler
else
ifneq ($(YQ2_OSTYPE), Darwin)
# Normalize some abiguous YQ2_ARCH strings
YQ2_ARCH ?= $(shell uname -m | sed -e 's/i.86/i386/' -e 's/amd64/x86_64/' -e 's/arm64/aarch64/' -e 's/^arm.*/arm/')
else
YQ2_ARCH ?= $(shell uname -m)
endif
endif

# On Windows / MinGW $(CC) is undefined by default.
ifeq ($(YQ2_OSTYPE),Windows)
CC ?= gcc
endif

# Detect the compiler
ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
COMPILER := clang
COMPILERVER := $(shell $(CC) -dumpversion | sed -e 's/[^0-9].*//')
else ifeq ($(shell $(CC) -v 2>&1 | grep -c -E "(gcc version|gcc-Version)"), 1)
COMPILER := gcc
COMPILERVER := $(shell $(CC) -dumpversion | sed -e 's/[^0-9].*//')
else
COMPILER := unknown
endif

# ----------

# Base CFLAGS. These may be overridden by the environment.
# Highest supported optimizations are -O2, higher levels
# will likely break this crappy code.
ifdef DEBUG
CFLAGS ?= -O0 -g -Wall -pipe
else
CFLAGS ?= -Wall -pipe -fomit-frame-pointer
endif

# Always needed are:
#  -fno-strict-aliasing since the source doesn't comply
#   with strict aliasing rules and it's next to impossible
#   to get it there...
#  -fwrapv for defined integer wrapping. MSVC6 did this
#   and the game code requires it.
override CFLAGS += -std=gnu99 -fno-strict-aliasing -fwrapv

# -MMD to generate header dependencies. Unsupported by
#  the Clang shipped with OS X.
ifneq ($(YQ2_OSTYPE), Darwin)
override CFLAGS += -MMD
endif

# OS X architecture.
ifeq ($(YQ2_OSTYPE), Darwin)
override CFLAGS += -arch $(YQ2_ARCH)
endif

# ----------

# Switch of some annoying warnings.
ifeq ($(COMPILER), clang)
	# -Wno-missing-braces because otherwise clang complains
	#  about totally valid 'vec3_t bla = {0}' constructs.
	CFLAGS += -Wno-missing-braces
else ifeq ($(COMPILER), gcc)
	# -Wno-missing-braces for the same reason as the clang branch above:
	# the id/mission-pack monster tables and 'vec3_t bla = {0}' constructs
	# are valid C; GCC's -Wmissing-braces is a pedantic false positive here.
	# (override: CFLAGS has override origin from line ~94, so a plain += here
	# would be silently dropped by GNU Make.)
	override CFLAGS += -Wno-missing-braces
	# GCC 8 or higher. COMPILERVER is just the leading digit run of
	# `-dumpversion` (major version only) -- GCC has reported only the bare
	# major version there since GCC 7 (e.g. "13", not "13.3.0"), and the
	# mingw-w64 cross compilers append a non-numeric suffix on top of that
	# ("13-win32"), so strip at the first non-digit rather than assume a
	# dotted X.Y.Z string to renormalize.
	ifeq ($(shell test $(COMPILERVER) -ge 8; echo $$?),0)
	    # -Wno-format-truncation and -Wno-format-overflow
		# because GCC spams about 50 false positives.
    	CFLAGS += -Wno-format-truncation -Wno-format-overflow
	endif
endif

# ----------

# Defines the operating system and architecture
override CFLAGS += -DYQ2OSTYPE=\"$(YQ2_OSTYPE)\" -DYQ2ARCH=\"$(YQ2_ARCH)\"

# ----------

# For reproduceable builds, look here for details:
# https://reproducible-builds.org/specs/source-date-epoch/
ifdef SOURCE_DATE_EPOCH
CFLAGS += -DBUILD_DATE=\"$(shell date --utc --date="@${SOURCE_DATE_EPOCH}" +"%b %_d %Y" | sed -e 's/ /\\ /g')\"
endif

# ----------

# Using the default x87 float math on 32bit x86 causes rounding trouble
# -ffloat-store could work around that, but the better solution is to
# just enforce SSE - every x86 CPU since Pentium3 supports that
# and this should even improve the performance on old CPUs
ifeq ($(YQ2_ARCH), i386)
override CFLAGS += -msse -mfpmath=sse
endif

# Force SSE math on x86_64. All sane compilers should do this
# anyway, just to protect us from broken Linux distros.
ifeq ($(YQ2_ARCH), x86_64)
override CFLAGS += -mfpmath=sse
endif

# Decompilation-noise suppressions.  These warning classes are all "authentic"
# to IDA's reconstruction of 32-bit x86 code and indicate no real defect:
#
#   int-conversion / incompatible-pointer-types / pointer-to-int-cast /
#   int-to-pointer-cast      — original x86 mixed 4-byte ptrs and ints freely.
#   unused-variable / unused-but-set-variable / unused-function
#                             — IDA emits more locals/helpers than its data-flow
#                               actually uses; trimming would diverge from the
#                               original allocation pattern.
#   pointer-sign              — char* / unsigned char* mismatches at function
#                               boundaries; identical in memory.
#   maybe-uninitialized / uninitialized
#                             — GCC's flow analysis can't follow IDA's goto-heavy
#                               control flow; original assembly always initialises.
#   parentheses / comment     — IDA emits expressions and string literals
#                               (e.g. "/* XXX impl */" comments inside docstrings)
#                               that GCC nags about; cosmetic.
#   stringop-overflow / stringop-overread / format-overflow / dangling-pointer
#                             — faithful reproductions of the original DLL's
#                               buggy-but-authentic patterns: strncat with a
#                               bound equal to dest size, sprintf into a fixed
#                               buffer with %s, returning &local.  Fixing them
#                               would diverge from the binary.
BOTCFLAGS=-O0 \
          -Wno-int-conversion -Wno-incompatible-pointer-types \
          -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -Wno-unused-but-set-variable -Wno-unused-function \
          -Wno-pointer-sign \
          -Wno-maybe-uninitialized -Wno-uninitialized \
          -Wno-parentheses -Wno-comment -Wno-misleading-indentation \
          -Wno-stringop-overflow -Wno-stringop-overread \
          -Wno-format-overflow -Wno-dangling-pointer

# ----------

# Base LDFLAGS.
LDFLAGS ?=

# It's a shared library.
override LDFLAGS += -shared

# Required libaries
ifeq ($(YQ2_OSTYPE), Darwin)
override LDFLAGS += -arch $(YQ2_ARCH)
else ifeq ($(YQ2_OSTYPE), Windows)
override LDFLAGS += -static-libgcc
# Strip stdcall @N decoration from exports so GetProcAddress("GetBotAPI")
# finds the symbol.  MinGW decorates __stdcall exports as _Name@N by default;
# --kill-at restores the undecorated name, matching the original MSVC .def
# file behaviour.
override LDFLAGS += -Wl,--kill-at
# Generate a linker map: maps every symbol to its address in the DLL.
# When the exception handler logs a crash address, look it up in this file.
# Each DLL link rule appends -Wl,-Map,<its-own-map> to LDFLAGS so the two
# linker invocations don't overwrite each other's output.  See the
# release/gladiator.dll and release/game/game.dll rules further down.
else
override LDFLAGS += -lm
endif

# ----------

# Builds everything
all: botlib game

# ----------

# When make is invoked by "make VERBOSE=1" print
# the compiler and linker commands.

ifdef VERBOSE
Q :=
else
Q := @
endif

# ----------

# Phony targets
.PHONY : all clean botlib game verify scan

# ----------

# Regression / quality gates
#
#   make verify  — runs every static scanner under tools/ and the
#                  struct-size cross-check.  Intended for CI.  Does NOT
#                  build the DLL; pair with `make botlib` to also verify
#                  the compile-time _Static_assert layout guards.
#
#   make scan    — alias for verify (legacy name)
#
# Each scanner exits non-zero on a hit, so any new bug-class regression
# fails the umbrella.  `check_struct_sizes.py` similarly fails if a
# documented `/* sizeof = N */` comment loses its matching assert.

verify scan:
	@echo "===> tools/scan_plane_stride.py"
	${Q}python3 tools/scan_plane_stride.py
	@echo "===> tools/scan_int_as_float_array.py"
	${Q}python3 tools/scan_int_as_float_array.py
	@echo "===> tools/scan_lodword_into_float.py"
	${Q}python3 tools/scan_lodword_into_float.py
	@echo "===> tools/scan_dropped_st0.py"
	${Q}python3 tools/scan_dropped_st0.py
	@echo "===> tools/scan_float_where_int_sig.py (informational; non-zero allowed)"
	-${Q}python3 tools/scan_float_where_int_sig.py
	@echo "===> tools/scan_vec3_splits.py"
	${Q}python3 tools/scan_vec3_splits.py
	@echo "===> tools/scan_fpu_temp_saturation.py (informational; non-zero allowed)"
	-${Q}python3 tools/scan_fpu_temp_saturation.py
	@echo "===> tools/scan_aarch64_ptr_trunc.py (skips when release/gladiator.so absent)"
	${Q}python3 tools/scan_aarch64_ptr_trunc.py
	@echo "===> tools/check_struct_sizes.py"
	${Q}python3 tools/check_struct_sizes.py
	@echo "===> verify OK"

# ----------

# Cleanup
clean:
	@echo "===> CLEAN"
	${Q}rm -Rf build release

# ----------

# The botlib library
ifeq ($(YQ2_OSTYPE), Windows)
botlib:
	@echo "===> Building gladiator.dll"
	${Q}mkdir -p release
	$(MAKE) release/gladiator.dll
else ifeq ($(YQ2_OSTYPE), Darwin)
botlib:
	@echo "===> Building gladiator.dylib"
	${Q}mkdir -p release
	$(MAKE) release/gladiator.dylib
else
botlib:
	@echo "===> Building gladiator.so"
	${Q}mkdir -p release
	$(MAKE) release/gladiator.so

release/gladiator.so : CFLAGS += -fPIC
endif

build/%.o: botlib/%.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) $(BOTCFLAGS) -DBOTLIB -DC_ONLY -o $@ $<


build/%.o: game/%.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) $(BOTCFLAGS) -Igame/ -o $@ $<

# Mr. Elusive's 1999 lcc.mak compiled q_shared.c to its own object file
# (q_shared.obj) and linked it into gladiator.dll alongside the bot
# sources.  Mirror that here: build game/q_shared.c with -DC_ONLY so its
# MSVC __asm paths (id386 Q_ftol, BoxOnPlaneSide fast path) compile out
# on gcc/MinGW.
build/q_shared.o: game/q_shared.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) $(BOTCFLAGS) -DC_ONLY -Igame/ -o $@ $<

build/game/%.o: game/%.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	# ZOID is NOT defined here: g_local.h defines it unconditionally (it is
	# the authoritative original definition), so a -DZOID on the command line
	# is redundant and only triggers a "ZOID redefined" warning (the -D form
	# expands to 1, g_local.h's to empty). Every game/*.c includes g_local.h.
	${Q}$(CC) -c $(CFLAGS) $(BOTCFLAGS) -Dstricmp=strcasecmp -DC_ONLY -Igame/ -o $@ $<

# game/bl_main.c's Win32 BotLoadLibrary() calls gladiator.dll's GetBotAPI
# through a function pointer typedef'd WINAPI (__stdcall), but the real
# export is cdecl (botlib.c's GetBotAPI ends in a plain `ret`, verified
# against the original disassembly -- gladq2_src/bl_main.c has the same
# WINAPI typedef, so this mismatch is authentic to the 1999 sources, not a
# reconstruction artifact). The __stdcall assumption makes the caller emit
# a compensating post-call `sub esp,4` that only the real callee's `ret 4`
# would have justified; with -fomit-frame-pointer every later ESP-relative
# local in BotLoadLibrary (incl. the `lib` pointer used right after) reads
# 4 bytes off, corrupting it -- crashes on every bot spawn. A frame pointer
# sidesteps this (those locals become EBP-relative instead), which is why
# it was never seen with MSVC6 or with MinGW toolchains whose vectorizer
# happens to force one for the adjacent struct copy -- that's incidental,
# not guaranteed (observed: MSYS2 gcc 15.2 does, plain i686-w64-mingw32-gcc
# 13 doesn't, and the latter reliably crashes without this), so force it
# explicitly instead of relying on it. Only matters on i386 (WINAPI is a
# no-op on x86_64 -- one calling convention for everything there), but
# apply for every Windows build regardless: YQ2_ARCH's value here isn't a
# reliable i386-vs-x86_64 signal by itself (e.g. a real MSYS2/MINGW32 shell
# with no MINGW_CHOST/PROCESSOR_ARCHITEW6432 set reports YQ2_ARCH=AMD64
# from PROCESSOR_ARCHITECTURE even though /mingw32/bin/gcc.exe only ever
# emits 32-bit PE32), and the flag is a harmless no-op on genuine x86_64.
ifeq ($(YQ2_OSTYPE), Windows)
build/game/bl_main.o : CFLAGS += -fno-omit-frame-pointer
endif

# ----------

# botlib.c is being progressively split back into Mr. Elusive's original
# translation units (l_crc.c, l_log.c, ...); the boundaries are recovered in
# .claude/memory/tu_partition.md.  Keep the split TUs in original .text order
# between botlib.o and the glue objects -- the same order build_oracle_dll.sh's
# SPLIT_TUS uses -- so both builds lay the image out the same way.
BOTLIB_OBJS_ = \
	botlib.o \
	be_aas_bspq2.o \
	be_aas_cluster.o \
	be_aas_debug.o \
	be_aas_entity.o \
	be_aas_file.o \
	be_aas_light.o \
	be_aas_main.o \
	be_aas_move.o \
	be_aas_optimize.o \
	be_aas_reach.o \
	be_aas_route.o \
	be_aas_routealt.o \
	be_aas_sample.o \
	be_aas_sound.o \
	be_ai2_dmnet.o \
	be_ai2_dmq2.o \
	be_ai2_main.o \
	be_ai_char.o \
	be_ai_chat.o \
	be_ai_goal.o \
	be_ai_move.o \
	be_ai_weap.o \
	be_ai_weight.o \
	be_ea.o \
	be_interface.o \
	l_crc.o \
	l_libvar.o \
	l_log.o \
	l_memory.o \
	l_precomp.o \
	l_script.o \
	l_struct.o \
	l_utils.o \
	botlib_debug.o \
	q_shared.o

# ----------

GAME_OBJS_ = \
	bl_botcfg.o \
	bl_cmd.o \
	bl_debug.o \
	bl_main.o \
	bl_redirgi.o \
	bl_spawn.o \
	dm_ball_rogue.o \
	dm_tag_rogue.o \
	g_ai.o \
	g_arena.o \
	g_ch.o \
	g_chase.o \
	g_cmds.o \
	g_combat.o \
	g_ctf.o \
	g_func.o \
	g_items.o \
	g_log.o \
	g_main.o \
	g_misc.o \
	g_monster.o \
	g_newai_rogue.o \
	g_newdm_rogue.o \
	g_newfnc_rogue.o \
	g_newtarg_rogue.o \
	g_newtrig_rogue.o \
	g_newweap_rogue.o \
	g_phys.o \
	g_save.o \
	g_spawn.o \
	g_sphere_rogue.o \
	g_svcmds.o \
	g_target.o \
	g_trigger.o \
	g_turret.o \
	g_utils.o \
	g_weapon.o \
	m_actor.o \
	m_berserk.o \
	m_boss2.o \
	m_boss3.o \
	m_boss31.o \
	m_boss32.o \
	m_boss5_xatrix.o \
	m_brain.o \
	m_carrier_rogue.o \
	m_chick.o \
	m_fixbot_xatrix.o \
	m_flash.o \
	m_flipper.o \
	m_float.o \
	m_flyer.o \
	m_gekk_xatrix.o \
	m_gladb_xatrix.o \
	m_gladiator.o \
	m_gunner.o \
	m_hover.o \
	m_infantry.o \
	m_insane.o \
	m_medic.o \
	m_move.o \
	m_move2_rogue.o \
	m_mutant.o \
	m_parasite.o \
	m_soldier.o \
	m_stalker_rogue.o \
	m_supertank.o \
	m_tank.o \
	m_turret_rogue.o \
	m_widow2_rogue.o \
	m_widow_rogue.o \
	p_botmenu.o \
	p_client.o \
	p_hud.o \
	p_lag.o \
	p_menu.o \
	p_menulib.o \
	p_observer.o \
	p_trail.o \
	p_view.o \
	p_weapon.o \
	q_shared.o

# ----------

# Rewrite pathes to our object directory
BOTLIB_OBJS = $(patsubst %,build/%,$(BOTLIB_OBJS_))
GAME_OBJS = $(patsubst %,build/game/%,$(GAME_OBJS_))

# ----------

# Generate header dependencies
DEPS= $(BOTLIB_OBJS:.o=.d)
GAME_DEPS= $(GAME_OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(DEPS)
-include $(GAME_DEPS)

# ----------

ifeq ($(YQ2_OSTYPE), Windows)
release/gladiator.dll : $(BOTLIB_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(BOTLIB_OBJS) $(LDFLAGS) -Wl,-Map,release/gladiator.map
else ifeq ($(YQ2_OSTYPE), Darwin)
release/gladiator.dylib : $(BOTLIB_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(BOTLIB_OBJS) $(LDFLAGS)
else
release/gladiator.so : $(BOTLIB_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(BOTLIB_OBJS) $(LDFLAGS)
endif

# ----------

# The gladiator game module
ifeq ($(YQ2_OSTYPE), Windows)
game:
	@echo "===> Building game.dll"
	${Q}mkdir -p release/game
	$(MAKE) release/game/game.dll

release/game/game.dll : $(GAME_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(GAME_OBJS) $(LDFLAGS) -Wl,-Map,release/game/game.map

else ifeq ($(YQ2_OSTYPE), Darwin)
game:
	@echo "===> Building game.dylib"
	${Q}mkdir -p release/game
	$(MAKE) release/game/game.dylib

release/game/game.dylib : CFLAGS += -fPIC
release/game/game.dylib : $(GAME_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(GAME_OBJS) $(LDFLAGS)

else
game:
	@echo "===> Building game.so"
	${Q}mkdir -p release/game
	$(MAKE) release/game/game.so

release/game/game.so : CFLAGS += -fPIC
release/game/game.so : $(GAME_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(GAME_OBJS) $(LDFLAGS)
endif

# ----------
