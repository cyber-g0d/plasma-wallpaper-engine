# plasma-wallpaper-engine

> **Fork of [CaptSilver/wallpaper-engine-kde-plugin](https://github.com/CaptSilver/wallpaper-engine-kde-plugin)**  
> Focus: plasmashell safety, stability, and defensive integration with KDE Plasma 6.

Live wallpapers from [Wallpaper Engine](https://store.steampowered.com/app/431960/Wallpaper_Engine), running natively in KDE Plasma 6. Scene, Web, and Video wallpapers all work, drawn by a custom Vulkan renderer — no emulation, no Windows code involved.

This fork builds on [CaptSilver/wallpaper-engine-kde-plugin](https://github.com/CaptSilver/wallpaper-engine-kde-plugin) with an emphasis on **safety-first integration**: protecting plasmashell from crashes, resource exhaustion, and misbehaving wallpapers.

📖 Upstream documentation: [CaptSilver Wiki](https://github.com/CaptSilver/wallpaper-engine-kde-plugin/wiki)

## License

**GPL-2.0** — same as upstream. See [LICENSE](./LICENSE) for the full text.

## ⚠️ Safety Limitations

This project works as a **KDE Plasma wallpaper plugin**. By design, it integrates deeply with the Plasma desktop environment. Current known limitations:

- **No plasmashell crash isolation yet** — misbehaving wallpapers can still crash plasmashell.
- **Renderer runs in-process** with the Plasma shell (Vulkan scene renderer, mpv/libmpv for video, Qt WebEngine for web wallpapers).
- **Resource usage is unbounded** — large scenes, high-framerate video wallpapers, or heavy Web wallpapers may degrade desktop responsiveness.
- **Steam integration is required** — Wallpaper Engine must be installed via Steam; wallpapers are loaded from the Steam library.

### What this fork aims to improve

| Area | Status |
|------|--------|
| Out-of-process rendering / sandboxing | Planned |
| Graceful wallpaper fallback on error | Planned |
| Resource limits (VRAM, CPU, FPS caps) | Planned |
| Safe wallpaper enumeration (no crashes from corrupt assets) | Under review |
| Per-wallpaper health metrics | Planned |

**Do not deploy on production/critical machines until safety milestones are reached.**

## Install

**⚠️ Development fork — no stable releases yet.** Build from source:

```sh
git clone --recurse-submodules https://github.com/cyber-g0d/plasma-wallpaper-engine.git
cd plasma-wallpaper-engine
git checkout dev/plasmashell-safety
cmake -B build -S .
cmake --build build
```

For upstream stable releases, use [CaptSilver/wallpaper-engine-kde-plugin](https://github.com/CaptSilver/wallpaper-engine-kde-plugin).

## Upstream

- **Original:** [catsout/wallpaper-engine-kde-plugin](https://github.com/catsout/wallpaper-engine-kde-plugin) (archived)
- **Active upstream:** [CaptSilver/wallpaper-engine-kde-plugin](https://github.com/CaptSilver/wallpaper-engine-kde-plugin)
- **This fork:** [cyber-g0d/plasma-wallpaper-engine](https://github.com/cyber-g0d/plasma-wallpaper-engine)

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md) and [ROADMAP.md](./ROADMAP.md).

> **Note:** Code patches are not accepted until upstream sources have been reviewed and reproducible tests exist for the targeted behavior. See [ROADMAP.md](./ROADMAP.md) for the review schedule.
