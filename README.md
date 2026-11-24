# Auto Head Gear

A native mod for [OpenRAC](https://github.com/OpenRAC/OpenRAC)'s port of Ratchet & Clank: the
**Sonic Summoner** and the **O2 Mask** work as soon as you own them, as if worn, like in the later
games. No more trips to the Items menu to put one on.

- **Sonic Summoner:** the sand mice answer it without it on Ratchet's head.
- **O2 Mask:** it keeps out Blarg's toxic gas and the gas of Rilgar's extermination robots without
  it on Ratchet's head.

[![Auto Head Gear in game (video)](docs/video-thumbnail.jpg)](https://youtu.be/rYhU7HuhehI)

*▶ [Watch it on YouTube](https://youtu.be/rYhU7HuhehI) (20 s, with sound).*

![Ratchet wearing the O2 Mask while a sand mouse leaves its house to follow him](docs/sand-mouse-with-o2-mask.png)

*The O2 Mask on, the Sonic Summoner only owned: the sand mouse still comes.*

Ratchet's head stays as it is: whatever you put on yourself keeps showing, and the game still puts
the mask on by itself underwater and in vacuum, as it always did. Each item has a switch in the
mod's settings.

**Keep chosen headgear on:** the headgear you put on in the Items menu (the Pilot's Helmet, the O2
Mask or the Sonic Summoner), or none, stays as you chose after a death, a planet change or a load,
the way the Grindboots and Magneboots do. Where the game puts the O2 Mask on by itself (underwater,
Blarg Station's airless parts, Orxon, Gemlik Base, Drek's Fleet), it still does, and your choice
comes back when you leave. An item you no longer own is not put back. It is cosmetic: what the
items do is the same as before. A switch in the settings, on by default.

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

**Keeping the headgear on.** The game keeps the items you have equipped in a list of seven item ids
beside the hero (PAL `0x141760`, NTSC `0x141660`): the Items menu writes your choice there, the
game's own save keeps the whole list, and each level start makes the hero's item slots from it. The
boots stay on because nothing clears their entry; the head's entry is cleared at every level start
(`reset_hero_to_idle`, called from the level's gameplay load), and a death, a planet change and a
load all start the level again. The mod reads that entry before each level start
(`rac1.level.load_gameplay`) and puts it back after, when it holds a head item you still own. It
writes nothing else and nothing while you play: the game's own O2 Mask never touches the entry (it
sets the worn item aside in the hero and gives it back itself when the need ends), so the forced
mask, cutscenes and vehicles are the game's as before. Your choice lives in the game's own save, so
it goes with each save file and needs no data of the mod's; a save loaded
(`rac1.save.restore_data`) brings its own. A tick's end only reads the entry, to log a new choice.
`src/keep_head.hpp` cites the game's code.

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
