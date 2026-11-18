# Auto Head Gear

A native mod for [OpenRAC](https://github.com/OpenRAC/OpenRAC)'s port of Ratchet & Clank: the
**Sonic Summoner** and the **O2 Mask** work as soon as you own them, as if worn, like in the later
games. No more trips to the Items menu to put one on.

- **Sonic Summoner:** the sand mice answer it without it on Ratchet's head.
- **O2 Mask:** it keeps out Blarg's toxic gas and the gas of Rilgar's extermination robots without
  it on Ratchet's head.

![Ratchet wearing the O2 Mask while a sand mouse leaves its house to follow him](docs/sand-mouse-with-o2-mask.png)

*The O2 Mask on, the Sonic Summoner only owned: the sand mouse still comes.*

Ratchet's head stays as it is: whatever you put on yourself keeps showing, and the game still puts
the mask on by itself underwater and in vacuum, as it always did. Each item has a switch in the
mod's settings.

For the PAL and NTSC-U discs, OpenRAC 1.1.0 or later (mod interface 1.20).

## Install

In the OpenRAC launcher's Mods tab, **Add from a link**, and paste
`https://github.com/Promises/rac1-auto-head-gear`.

## How it works

The game decides these things by the item in the head slot (`g_hero.items[2]`), inside the update
of each moby that asks: the sand mice (classes 437 and 1818) and their houses (604) for the Sonic
Summoner, the toxic gas vents (911) and the extermibot (623) and extermitank (625) for the O2 Mask.
The mod wraps those updates: for the length of one, when you own the item and wear something else,
the head slot reads as that item, then it is put back exactly as it was. The sand mice also want
the Summoner's model on the head; with nothing there Ratchet's own moby stands in, which the game
only reads after checking it is a Summoner, so nothing is drawn or moved differently.

The hook points are the SDK's moby updates, which it marks experimental. The addresses and the
checks are cited in `src/layout.hpp` and `src/mod.cpp`.

## Build

```sh
python3 tools/fetch_sdk.py          # the SDK sdk.lock pins, into .sdk/
cmake -S . -B build -G Ninja
cmake --build build && ctest --test-dir build
python3 .sdk/openrac-mod-sdk-*/tools/package_mod.py build/package/auto_head_gear
```

A tag `v<version>` (the manifest's) makes the workflow build every platform and publish the
release the launcher installs from.

## License

MIT.
