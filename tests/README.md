# Regression tests

From the repository root, with Python 3 and a GCC/Clang-compatible C++17 compiler:

```sh
python3 tests/run_tests.py
```

For Clang with AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
CXX=clang++ CXXFLAGS='-fsanitize=address,undefined -g' python3 tests/run_tests.py
```

The runner extracts the current `Trace` definition, constants, and affected method bodies from `Src/` into
a temporary C++ harness. The production logic is not copied into the tests. Engine entities, weapon firing, position reads,
and frame-callback removal are test doubles because the SDK and Windows game cannot run on every test host.
Generated files and binaries are removed after the run. Compilation errors or failed checks return nonzero.

Coverage includes the production firing detour, setup queue, and rendering filter: ordinary darts/bullets,
wall-stopped bullets, immediate-impact dart ammo, nested firing and unattributed setup, projectile invalidation, address reuse during play
and pause, stationary shots, and deactivation/deletion order on unload. These checks cannot validate vtable
addresses, the Windows SEH position reader, engine threading, or the game's entity/ability implementation.

Before release, build the DLL against ZHMModSDK 4.2.1 on Windows and check in HITMAN 3.290.1:

1. Enable infinite ammo, then disable BulletTracer through the SDK Mods list. Ammo should return to normal
   when no other mod or game mechanic provides infinite ammo. Re-enable BulletTracer and repeat.
2. Repeat with infinite ammo already off, then with a mission restart and a level change.
3. Fire darts at nearby and distant surfaces. In default darts-only mode, measured short flights should remain
   classified as darts, even with no movement samples (a line still needs two distinct points). Include Sieker,
   tranquilizer, Serpent's Bite, and cure darts. Confirm setup runs synchronously inside the firing hook.
4. Fire successive shots while earlier traces are still active. Previous lines must not jump to the next shot.
5. Pause/resume through FreeCam during flight. The trace should resume without a false landing or extra segment.
6. Fire normal bullets next to walls, including when the hand clips into a wall. Darts seen must not increase;
   lines must appear only with Trace all shots on. Check both depth-rendering modes and NPC gunfire.

## SDK evidence and remaining integration checks

The firing hook signature and pattern come from ZHMModSDK v4.2.1 (`70fae621`),
`ZHMModSDK/Include/Hooks.h` and `ZHMModSDK/Src/Hooks.cpp`. `Glacier/ZItem.h` declares
`ZHM5ItemWeapon` as an `IFirearm` and exposes `GetAmmoType()`; `Glacier/Enums.h` defines
`eAmmoDartTranquilizer`, `eAmmoDartEmetic`, and `eAmmoDartCure`. No new projectile or
setup-parameter memory offsets are assumed. The existing projectile vtable/position offsets are unchanged.

Portable tests simulate synchronous setup during firing. They cannot establish that all game projectile
launches use that call path, or validate the SDK's runtime virtual layout. Verify those points with the
Windows build and in-game checks above before release. Unattributed setups deliberately count as other
shots rather than falling back to ambiguous sampled speed.
