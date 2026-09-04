# Contributing to plasma-wallpaper-engine

## 🚧 Current Phase: Source Review

We are in the **initial source review phase**. No code patches are being accepted yet. Here's what you can do:

### What's needed now

1. **Reproduce crashes** — find wallpapers or configurations that crash plasmashell, document exact steps.
2. **Review the source** — read through `src/`, identify:
   - Unsafe assumptions about plasmashell lifecycle
   - Missing error handling in wallpaper loading/rendering paths
   - Resource leaks (VRAM, file descriptors, threads)
   - Signal-slot chains that could trigger cascading failures
3. **Write reproducible tests** — for any identified issue, create a minimal reproducer.
4. **Discuss architecture** — propose designs for out-of-process rendering, sandboxing, and resource limits in [Discussions](https://github.com/cyber-g0d/plasma-wallpaper-engine/discussions).

### When code patches will be accepted

After each source area has been reviewed and at least one reproducible test exists:

- `src/` — wallpaper enumeration, loading, lifecycle
- `plugin/` — KDE plugin integration, plasmashell interface
- `src/backend_scene/` — Vulkan renderer safety

See [ROADMAP.md](./ROADMAP.md) for the review schedule.

## Build and Test

```sh
git clone --recurse-submodules https://github.com/cyber-g0d/plasma-wallpaper-engine.git
cd plasma-wallpaper-engine
git checkout dev/plasmashell-safety
cmake -B build -S .
tools/scripts/preflight.sh   # lint + build + tests + fuzz smoke
```

⚠️ **Do not install the plugin on a machine you depend on.** Test in a VM or a dedicated test user account.

## Reporting Issues

Use [GitHub Issues](https://github.com/cyber-g0d/plasma-wallpaper-engine/issues) with the following templates:

- **Bug Report** — crashes, hangs, visual glitches
- **Safety Audit Finding** — code paths with missing safety guards
- **Reproducer** — minimal wallpaper/setup that triggers a bug

Include:
- KDE Plasma version (`plasmashell --version`)
- Qt version
- GPU + driver version
- Wallpaper Workshop ID (if applicable)
- Steps to reproduce
- Backtrace if plasmashell crashed (`coredumpctl`)

## Upstream

This is a fork of [CaptSilver/wallpaper-engine-kde-plugin](https://github.com/CaptSilver/wallpaper-engine-kde-plugin) (GPL-2.0). Contributions that are not safety-specific should go upstream. Safety improvements will be offered back as patches.
