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

# Detect the architecture
ifeq ($(YQ2_OSTYPE), Windows)
ifdef MINGW_CHOST
ifeq ($(MINGW_CHOST), x86_64-w64-mingw32)
YQ2_ARCH ?= x86_64
else # i686-w64-mingw32
YQ2_ARCH ?= i386
endif
else # windows, but MINGW_CHOST not defined
ifdef PROCESSOR_ARCHITEW6432
# 64 bit Windows
YQ2_ARCH ?= $(PROCESSOR_ARCHITEW6432)
else
# 32 bit Windows
YQ2_ARCH ?= $(PROCESSOR_ARCHITECTURE)
endif
endif # windows but MINGW_CHOST not defined
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
COMPILERVER := $(shell $(CC)  -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/')
else ifeq ($(shell $(CC) -v 2>&1 | grep -c -E "(gcc version|gcc-Version)"), 1)
COMPILER := gcc
COMPILERVER := $(shell $(CC)  -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/')
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
	# GCC 8.0 or higher.
	ifeq ($(shell test $(COMPILERVER) -ge 80000; echo $$?),0)
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
override LDFLAGS += -Wl,-Map,release/gladiator.map
else
override LDFLAGS += -lm
endif

# ----------

# Builds everything
all: gladiator

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
.PHONY : all clean gladiator

# ----------

# Cleanup
clean:
	@echo "===> CLEAN"
	${Q}rm -Rf build release

# ----------

# The gladiator game
ifeq ($(YQ2_OSTYPE), Windows)
gladiator:
	@echo "===> Building gladiator.dll"
	${Q}mkdir -p release
	$(MAKE) release/gladiator.dll
else ifeq ($(YQ2_OSTYPE), Darwin)
gladiator:
	@echo "===> Building gladiator.dylib"
	${Q}mkdir -p release
	$(MAKE) release/gladiator.dylib
else
gladiator:
	@echo "===> Building gladiator.so"
	${Q}mkdir -p release
	$(MAKE) release/gladiator.so

release/gladiator.so : CFLAGS += -fPIC
endif

build/%.o: botlib/%.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) $(BOTCFLAGS) -DBOTLIB -o $@ $<


build/%.o: game/%.c
	@echo "===> CC $<"
	${Q}$(CC) -c $(CFLAGS) $(BOTCFLAGS) -DBOTLIB -o $@ $<

# ----------

OBJS_ = \
	botlib.o \
	botlib_exports.o \
	botlib_debug.o \
	botlib_structdefs.o

# ----------

# Rewrite pathes to our object directory
OBJS = $(patsubst %,build/%,$(OBJS_))

# ----------

# Generate header dependencies
DEPS= $(OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(DEPS)

# ----------

ifeq ($(YQ2_OSTYPE), Windows)
release/gladiator.dll : $(OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(OBJS) $(LDFLAGS)
else ifeq ($(YQ2_OSTYPE), Darwin)
release/gladiator.dylib : $(OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(OBJS) $(LDFLAGS)
else
release/gladiator.so : $(OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(OBJS) $(LDFLAGS)
endif

# ----------
