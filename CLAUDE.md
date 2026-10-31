# dnd-server

A networked dice-rolling and session server for tabletop RPGs. Multiple clients
connect to an authoritative server; rolls are resolved server-side and broadcast
to everyone in the room, with campaign history persisted to SQLite.

This is a portfolio project. Its purpose is to demonstrate C++, network
programming, concurrency with shared state, and self-designed persistence.
The D&D framing is the domain, not the interesting part — the systems work is.

## Stack

- **Language:** C++20
- **Build:** CMake 3.21+, Ninja generator, MSVC (Visual Studio Build Tools 2026)
- **Dependencies:** vcpkg in manifest mode (`vcpkg.json`)
  - `asio` (standalone, not Boost)
  - `nlohmann-json`
  - `sqlite3` (vcpkg's `unofficial-sqlite3` config)
  - `catch2` (v3)
- **Client:** terminal CLI first, Qt Widgets later
- **Platform:** Windows 11, developed in VS Code

## Deliberate design decisions

These were chosen on purpose. Don't refactor away from them without asking.

**Blocking Asio, not async.** One `std::jthread` per connected client, with a
mutex around shared room state. Async Asio's completion handlers and
`shared_from_this` lifetime management are a large complexity cost for a project
this size, and blocking sockets read top-to-bottom like ordinary code. The
threading is real either way.

**Line-delimited JSON over the wire**, not length-prefixed binary. One JSON
object per line, terminated with `\n`. Chosen so the protocol can be debugged
with `nc` and read by eye.

**SQLite handled directly**, not through an ORM. Designing the schema and
writing the queries is part of the point.

**`common/` is a separate library** shared by server and clients, so the dice
parser and message types are testable without opening a socket.

## Repository layout

```
dnd-server/
├── CMakeLists.txt        # root; find_package + add_subdirectory
├── vcpkg.json            # manifest
├── .vscode/settings.json # vcpkg toolchain, Ninja, compile_commands
├── common/               # dice parser, protocol message types
├── server/               # listener, session threads, room state, db
├── client-cli/           # terminal client (not yet created)
├── client-qt/            # Qt client (phase 5)
└── tests/                # Catch2
```

`build/` and `vcpkg_installed/` are gitignored.

## Protocol

Client → server:

```json
{"type":"join","name":"Somil"}
{"type":"roll","expr":"2d6+3"}
{"type":"chat","text":"I check for traps"}
```

Server → clients (broadcast):

```json
{"type":"player_joined","name":"Somil"}
{"type":"roll_result","player":"Somil","expr":"2d6+3","dice":[4,6],"total":13,"modifier":3}

{"type":"chat","player":"Somil","text":"I check for traps"}
```

Server → one client (in reply to a bad message — invalid JSON, unknown type,
bad dice expression, rolling before joining):

```json
{"type":"error","message":"unknown dice expression"}
```

Roll results carry the individual dice, not just the total, so clients can
display `[4, 6] + 3 = 13`.

## Database schema

```sql
campaigns(id, name, created_at)
players(id, campaign_id, name)
rolls(id, campaign_id, player_id, expr, result_json, created_at)
```

Every roll is written. On join, the server replays the last N rolls to the
new client so late arrivals see history.

SQLite handles must not be shared carelessly across threads — either guard a
single handle with its own mutex or open one connection per thread.

## Current state

Phase 1 complete. Dice parses, returning an optional rollresult

## Roadmap

- [x] **Phase 1 — Dice parser.** Pure logic in `common/`. Parse `2d6+3`, `d20`,
      `4d8-2`. Return individual dice plus total. Catch2 tests covering
      malformed input, zero dice, absurd counts (`999999d6` must be rejected,
      not hang), negative modifiers.
- [ ] **Phase 2 — Echo server + CLI client.** `acceptor.accept()` loop,
      `std::jthread` per connection, line-delimited JSON. Verify with `nc`
      before writing the client.
- [ ] **Phase 3 — Room broadcast.** `Room` holding
      `std::vector<std::shared_ptr<Session>>` behind a `std::mutex`. Handle
      mid-broadcast disconnects by marking sessions dead and reaping them —
      do not erase from the vector while iterating it.
- [ ] **Phase 4 — Persistence.** Schema above, roll history, replay on join.
- [ ] **Phase 5 — Qt client.** Qt Widgets, `QTcpSocket` (not Asio client-side,
      so it integrates with Qt's event loop and needs no client threading).
      Keep the CLI client — it proves the protocol is client-agnostic.
- [ ] **Phase 6 — Finish.** README with architecture notes, demo GIF, GitHub
      Actions running the Catch2 suite on push.

Explicitly out of scope: authentication beyond a display name, distributed
anything, a web client, character sheets, combat rules.

## Build commands

Presets live in `CMakePresets.json`: `default` (Debug, `build/`) and `release`
(Release, `build-release/`, used by CI).

```powershell
# Configure (VS Code: CMake: Configure)
cmake --preset default

# Build
cmake --build --preset default

# Test
ctest --preset default
```

In VS Code, prefer the CMake Tools status bar and command palette
(`CMake: Configure`, `CMake: Build`, `CMake: Run Without Debugging`).
CMake Tools supplies the MSVC environment itself, so the presets just work there.

**From a plain terminal**, the Ninja generator needs `cl.exe` on `PATH`, so run
from a Developer PowerShell / Developer Command Prompt, or `call` this first:

```
C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat
```

**Gotcha:** `vcvars64.bat` overwrites `VCPKG_ROOT` with Visual Studio's own
bundled vcpkg. That copy is not a git checkout, so it can only resolve the
manifest because `vcpkg.json` pins `builtin-baseline` — but it rebuilds every
port from source instead of hitting the binary cache. To use the fast path, set
`VCPKG_ROOT=C:\vcpkg` _after_ sourcing vcvars.

## Conventions

- C++20. Prefer standard library over third-party.
- RAII everywhere; no raw `new`/`delete`.
- `snake_case` for functions and variables, `PascalCase` for types.
- Headers use `#pragma once`.
- Every new piece of logic in `common/` gets Catch2 tests in the same commit.
- Small commits with real messages.

## Working agreement

Plumbing is fair game to generate: CMake, vcpkg config, SQLite boilerplate,
Qt layout code, JSON serialization glue, README scaffolding.

The dice parser, the threading model, and the disconnect handling should be
written by hand, or read closely enough afterward that they could be rewritten
from scratch. These are the parts an interviewer will drill into, and "I'd have
to look at the code" about this project's core is worse than not having the
project.

When suggesting an approach, favor the simplest thing that works over the
most impressive. A finished project beats an ambitious unfinished one.
Always give hints or talk in plain english before giving code snippets, unless asked
