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

The reconstruction is **playable but a work in progress**.  Bots load,
spawn, fight and chat.  Some map features still trip them up, and rough
edges remain.  If you'd like to help test, report bugs or contribute, head
to the issue tracker.

## How close are we to the original?

To measure progress objectively we run an *oracle check*: we recompile our
reconstructed C source with the same 1999-era Microsoft Visual C++ 6
compiler that built the original `gladiator.dll`, and then compare the
resulting machine code against the bytes inside the real 1999 DLL — one
small routine at a time.

**Of the 696 routines in the original Gladiator Bot DLL, 516 (about
74 %) currently come out byte-for-byte identical to Mr. Elusive's
original.**

The remaining 180 are close but not yet exact: most of them are off by
just a handful of CPU instructions — usually a tiny source-level detail
we haven't pinned down yet — and **none are missing** (every routine in
the original is paired with one in our reconstruction). Each remaining
gap is a concrete, measurable target for further work.

## A note on "version 0.96"

Mr. Elusive shipped *two* builds under the same v0.96 label:

- **Windows** (`gladiator.dll` + `gamex86.dll`) — released **1999-07-18**.
- **Linux**  (`gladi386.so`  + `gamei386.so`)  — released **1999-08-02**,
  about two weeks later.

Despite the matching version number, the Linux build is the **more advanced
botlib**.  In particular, the Linux `gladi386.so` contains a roughly 7 KB
moving-brush reachability builder (`F149`, for `func_plat` / `func_train`
movers) that has **no counterpart in the Windows DLL**, and overall calls
**11** reach-type handlers vs. the Windows DLL's **6** — about **2.5×**
more reach-handler code by byte count.  In effect the Linux drop is a
quiet point-release that the Windows binary never received.

The map-prep tool `bspc` is the exception: the version bundled with the
Linux drop (`bspc-linux-x86`, **v1.2**, dated **1999-05-20**) is actually
*older* than the Windows `bspc.exe` (**v1.4**, dated **1999-07-18**) — the
opposite direction from the botlib.  This reconstruction is grounded in
the **Windows** DLL per the project's fidelity rules, so the extra
Linux-only botlib code is deliberately *out of scope* for the byte-level
match, but it is occasionally a useful secondary reference.

## Credits

- **Mr. Elusive** — original Gladiator Bot author (1999)
- **Squatt** and **Mr. Freeze** — original co-creators
- The **Yamagi Quake II** team for keeping the engine alive

## Licensing Rationale

This project is a reconstruction of the original Gladiator Bot source code from the Quake II era. The original source code was never publicly released, and only binary distributions are known to exist.

A substantial portion of the Gladiator Bot technology and codebase was later incorporated into the Quake III Arena bot system. The Quake III Arena source code was subsequently released under the GNU General Public License version 2 (GPLv2), making many of the underlying bot components and algorithms available under GPLv2 terms.

Based on the significant code lineage between the original Gladiator Bot and the later GPLv2-released Quake III Arena bot code, this reconstruction project is distributed under the GPL. Our intention is to preserve, study, and continue the development of this historically important software within the open-source community and in a manner consistent with the later GPLv2 release of related code.

This project does not claim ownership of the original work. If additional information regarding copyright ownership, licensing history, or rights transfers becomes available, the project's licensing and distribution terms may be reviewed and updated accordingly.

