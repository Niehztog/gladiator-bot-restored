# Gladiator Bot for Quake II — Restoration Project

In 1999, a programmer known as **Mr. Elusive** released the **Gladiator Bot** —
a free add-on that gave Quake II its first taste of intelligent
computer-controlled opponents.  Suddenly, single-player Quake II felt like a
LAN party.  Eighteen named bots with their own personalities, voices and play
styles roamed the maps, fragging each other and trash-talking in chat.

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
- **Reverse-engineering reference** (`reference/`) — the decompiler output
  used as a cross-check during reconstruction (not compiled, not the
  finished work, kept for transparency)

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

## Credits

- **Mr. Elusive** — original Gladiator Bot author (1999)
- **Squatt** and **Mr. Freeze** — original co-creators
- The **Yamagi Quake II** team for keeping the engine alive

## License

This project is distributed under the **original 1999 Gladiator Bot
license** — the same terms Mr. Elusive set out when he released the bot
as freeware: free to play, modify, share and tinker with; never to be
sold.  Commercial use must be negotiated directly with the copyright
holder.

The full terms are quoted verbatim in [LICENSE](LICENSE), and the
original 1999 readmes they came from are bundled in
[`game/readme.txt`](game/readme.txt) and
[`assets/readme.htm`](assets/readme.htm).
