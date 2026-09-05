# Contributing to plasma-wallpaper-engine

## 🚧 Current Phase: Defensive Hardening (Phase 1)

We are in the **Phase 1 defensive hardening phase**. Safety patches are now accepted. Here's what you can do:

### What's needed now

1. **Reproduce crashes** -- find wallpapers or configurations that crash plasmashell, document exact steps.
2. **Review the source** -- read through `src/`, identify:
   - Unsafe assumptions about plasmashell lifecycle
   - Missing error handling in wallpaper loading/rendering paths
   - Resource leaks (VRAM, file descriptors, threads)
   - Signal-slot chains that could trigger cascading failures
3. **Write reproducible tests** -- for any identified issue, create a minimal reproducer.
4. **Discuss architecture** -- propose designs for out-of-process rendering, sandboxing, and resource limits in [Discussions](https://github.com/cyber-g0d/plasma-wallpaper-engine/discussions).

### When code patches will be accepted

Phase 1 safety patches are now accepted. Focus areas:

- `src/backend_scene/qml_helper/` -- TextureNode lifecycle, SceneBackend safety
- `src/` -- wallpaper enumeration, loading, lifecycle, error recovery
- `plugin/` -- KDE plugin integration, plasmashell interface safety

Safety patches land in `dev/plasmashell-safety` (main fork) or `dev/null-texture-guard` (renderer fork submodule). Use the appropriate branch for the scope of change.

## Repositories

| Repo | Role | Branch |
|------|------|--------|
| [plasma-wallpaper-engine](https://github.com/cyber-g0d/plasma-wallpaper-engine) | Main fork | `dev/plasmashell-safety` |
| [wallpaper-scene-renderer](https://github.com/cyber-g0d/wallpaper-scene-renderer) | Renderer fork | `dev/null-texture-guard` |
| [CaptSilver/wallpaper-engine-kde-plugin](https://github.com/CaptSilver/wallpaper-engine-kde-plugin) | Upstream | `main` |
| [CaptSilver/wallpaper-scene-renderer](https://github.com/CaptSilver/wallpaper-scene-renderer) | Upstream renderer | `main` |

## Build and Test

```sh
# Main plugin build (requires KDE Plasma 6 + Qt6)
git clone --recurse-submodules https://github.com/cyber-g0d/plasma-wallpaper-engine.git
cd plasma-wallpaper-engine
git checkout dev/plasmashell-safety
cmake -B build -S .

# Renderer standalone tests (no Plasma deps needed)
cd src/backend_scene
cmake -B build_test -S . -DBUILD_TESTS=ON
cmake --build build_test -j$(nproc)
cd build_test && ctest --output-on-failure
```

⚠️ **Do not install the plugin on a machine you depend on.** Test in a VM or a dedicated test user account.

## Reporting Issues

Use [GitHub Issues](https://github.com/cyber-g0d/plasma-wallpaper-engine/issues) with the following templates:

- **Bug Report** -- crashes, hangs, visual glitches
- **Safety Audit Finding** -- code paths with missing safety guards
- **Reproducer** -- minimal wallpaper/setup that triggers a bug

Include:
- KDE Plasma version (`plasmashell --version`)
- Qt version
- GPU + driver version
- Wallpaper Workshop ID (if applicable)
- Steps to reproduce
- Backtrace if plasmashell crashed (`coredumpctl`)

## Upstream

This is a fork of [CaptSilver/wallpaper-engine-kde-plugin](https://github.com/CaptSilver/wallpaper-engine-kde-plugin) (GPL-2.0). Contributions that are not safety-specific should go upstream. Safety improvements will be offered back as patches.
