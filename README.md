# Gladiator Bot for Quake II — Restoration Project

First released on **December 8, 1998**, the
**[Gladiator Bot](https://mrelusive.com/oldprojects/gladiator/gladiator.html)** —
created by the Dutch programmer **Jan Paul "Mr. Elusive" van Waveren** — gave
Quake II its first taste of intelligent computer-controlled opponents.
Suddenly, single-player Quake II felt like a LAN party.  Eighteen named bots
with their own personalities, voices and play styles roamed the maps, fragging
each other and trash-talking in chat.

The Gladiator Bot was groundbreaking.  It was the first bot able to navigate
*any* Quake II map automatically, without a level designer hand-placing
waypoints — a technique Mr. Elusive later refined into the bots that ship
with Quake III Arena.  For many players, this is the bot they grew up with.

But there was always one catch: the Gladiator Bot's brain was closed source.
While Mr. Elusive released the source code for the *game module*, the actual
intelligence — the navigation, the decision-making, the chat system — lived
inside a sealed binary called `gladiator.dll`.  As the years go by, that
binary becomes harder and harder to keep running on modern systems.

This project is an effort to **bring the Gladiator Bot back from the
brink** by reconstructing its source code from the original Windows binary,
function by function, line by line.  Once complete, the Gladiator Bot will
be open, modifiable and portable — playable on Linux, macOS and modern
Windows for as long as people want to play Quake II.

The guiding rule is **reconstruct, don't invent**. Every function is
recovered from a disassembly of the real `gladiator.dll`, cross-checked
against several independent decompilations and against the Quake III Arena
bot source Mr. Elusive wrote next — the direct descendant of this same
code — and accepted only once it compiles back down to the *same machine
code* as the original (see [Authenticity](#authenticity) below). Original
bugs are preserved rather than fixed, and nothing is guessed when the
disassembly can answer instead.

> **Sister project:** If you're after a more advanced bot, see
> **[q3a_bot_backport_for_q2](https://github.com/Niehztog/q3a_bot_backport_for_q2)**
> — a Quake II adaptation of the Quake III Arena bot, the evolved successor to
> the Gladiator Bot's navigation technology.

## What's in the box

This repository bundles everything you need:

- **The reconstructed bot brain** (`botlib/`) — the work-in-progress port of
  the closed-source `gladiator.dll`
- **The original game module source** (`game/`) — Mr. Elusive's 1999
  game.dll source, included verbatim with attribution
- **The runtime assets** (`assets/`) — bot characters, voices, the things
  that make Adrenaline Hunk *feel* like Adrenaline Hunk
- **The map-prep tool** (`tools/`) — `bspc`, the utility that lets you
  teach the bots a new map

## What you get

- **18 classic bot characters** with distinct skins, names and personalities
  — Adrenaline Hunk, Laura Craft, Reaper, Maxine and the rest of the gang
- **Smart deathmatch opponents** that learn the map, hunt for items, dodge
  rockets and trash-talk in chat
- **Capture The Flag and team play** with bots that defend, attack and
  follow orders
- **Mission pack support** for *The Reckoning* and *Ground Zero*
- **Adjustable difficulty** so you can tune the bots to your skill level

## Why this matters

The Gladiator Bot is a piece of gaming history.  It marks the birth of the
navigation technology that powers bots in dozens of games to this day.
Reconstructing it preserves that history — and gives the Quake II community
a maintainable, future-proof bot library for the decades ahead.

## Status

The reconstruction is **feature-complete and behaviorally identical to the
original**.  Bots load, spawn, navigate, fight, chat and play Capture The
Flag exactly as they did in 1999 — the original Gladiator Bot experience is
fully reconstructed.  What remains are minor 64-bit and platform-conversion
edge cases, not missing behavior; if you hit one, the issue tracker is the
place to report it.

## Authenticity

Progress isn't self-reported: every reconstructed function is rebuilt with
the same compiler the original release used, then checked
instruction-for-instruction against the shipped 1999 binary. Mr. Elusive
shipped **two** independent v0.96 builds (see below), so there are two
independent oracles:

- **Windows** — the primary target. `gladiator.dll` is rebuilt with
  **Microsoft Visual C++ 6.0** (the original 1998 RTM release, identified
  from the DLL's own PE Rich header — not a later service pack, which
  measurably changes the generated code). Of **820** routines, **814
  (99%)** come out byte-identical machine code. The other six differ only
  in register allocation or instruction scheduling, by at most two
  instructions. **Nothing missing, nothing invented.**
- **Linux** — a second, independent channel. `gladi386.so` is rebuilt with
  **gcc 2.7.2.3**, the compiler recorded inside the 1999 binary itself. Of
  **810** routines, **807 (99.6%)** are byte-identical; the other three
  differ in register allocation and scheduling, not in logic. Nothing
  missing here either. The game module shipped beside it goes one step
  further: built from this repository's `game/`, it reproduces the 1999
  `gamei386.so` byte for byte, the whole file and not just its routines.

Both oracles compile the *same* source text; the next section explains
what that means for the two releases. Each remaining gap is a concrete,
measurable target, not a guess — the counts above come from a per-routine
audit against both oracles, so they only move when the source actually
does.

## A note on "version 0.96"

Mr. Elusive shipped v0.96 for two platforms, two weeks apart:

- **Windows** (`gladiator.dll` + `gamex86.dll`) — built **1999-07-18**.
- **Linux** (`gladi386.so` + `gamei386.so`) — built **1999-08-02**, in a
  glibc and a libc5 variant.

They are the same bot. Both packages carry the same readme ("version 0.96,
18th July 1999"), byte-identical bot data (`pak7.pak`) and the same map
tool, BSPC 1.4 (`bspc.exe` on Windows, `bspci386` on Linux). One source
text, the one in this repository, compiles to both bot libraries, and
every routine in the Linux library has its Windows counterpart, apart from
a few small helpers the Windows compiler folded into their callers. The
navigation code in particular is identical, down to the same ten
reachability builders, the routines that work out how a bot can get from
one area of a map to the next. No source change between the two builds
has been found. (Earlier versions of this README credited Linux with an
extra reachability builder, `F149`. That was a misidentification: `F149`
is `AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge`, which the
Windows DLL has too.)

What does differ is the layer each build uses to talk to its platform:

- **Missing navigation files.** When a map has no `.aas` file, the Windows
  build also looks inside `aas0.zip` … `aas9.zip`, through the Info-ZIP
  `unzip32.dll` its installer ships. With `autolaunchbspc 1` it then starts
  WinBSPC, a graphical front end for BSPC, to build the file. The Linux
  build does neither. Mr. Elusive's own readme says that zipping `.aas`
  files "only works with the Windows version of the Gladiator bot", and
  with `autolaunchbspc 1` the Linux build just prints "the BSPC tool is a
  Win32 program". This reconstruction keeps the Windows zip search, which
  needs `unzip32.dll` from the original installer (it is not shipped here)
  and a 32-bit build to load it, and it starts WinBSPC through the same C
  runtime call as the 1999 DLL.
- **Random numbers.** Both builds take `rand()` from the C library, but it
  is wired differently. On Windows, `gladiator.dll` and `gamex86.dll` each
  carry a private copy of the C runtime. The bot library seeds its copy
  from the clock. Nothing seeds the game library's copy, and the `rand()`
  call id's engine makes every server frame, to keep randomness
  time-dependent, cannot reach it. So the game library's draws, such as
  which bot `bots_minplayers` or `addrandom` picks next, start from
  the same state every time the server starts. On Linux the engine and
  both libraries draw from one shared generator, so the clock seed and the
  engine's draws reach the game library too. (This project's own Windows
  builds use the shared `msvcrt.dll`, so they behave like Linux here.)
- **Files and paths.** Backslashes versus forward slashes, `gladiator.dll`
  versus `gladi386.so` as the default `botlib`, and the Win32 or POSIX call
  that finds `bots/*.cfg`. None of this changes play.
- **Machine code.** Two different compilers turned the same source into
  different instructions. The logic is the same, and so are the 1999 bugs
  this project preserves in the shared code. But where one of those bugs
  reads uninitialized or out-of-bounds memory, it finds whatever that
  build's stack layout and C library left there, so its symptoms can
  differ between the two.

## Credits

- **Mr. Elusive** — original Gladiator Bot author (1999)
- **Squatt** and **Mr. Freeze** — original co-creators
- The **Yamagi Quake II** team for keeping the engine alive

## Licensing Rationale

This project is a reconstruction of the original Gladiator Bot source code from the Quake II era. The original source code was never publicly released, and only binary distributions are known to exist.

A substantial portion of the Gladiator Bot technology and codebase was later incorporated into the Quake III Arena bot system. The Quake III Arena source code was subsequently released under the GNU General Public License version 2 (GPLv2), making many of the underlying bot components and algorithms available under GPLv2 terms.

Based on the significant code lineage between the original Gladiator Bot and the later GPLv2-released Quake III Arena bot code, this reconstruction project is distributed under the GPL. Our intention is to preserve, study, and continue the development of this historically important software within the open-source community and in a manner consistent with the later GPLv2 release of related code.

This project does not claim ownership of the original work. If additional information regarding copyright ownership, licensing history, or rights transfers becomes available, the project's licensing and distribution terms may be reviewed and updated accordingly.

## Related projects

- **[Q2GladBot-Recon](https://github.com/themuffinator/Q2GladBot-Recon)** —
  an independent effort to reconstruct the Gladiator Bot, working toward the
  same goal from a different starting point.

