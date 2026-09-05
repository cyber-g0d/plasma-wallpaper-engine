# Changelog — plasma-wallpaper-engine

## [unreleased] — dev/plasmashell-safety

### Planned

- **Razer Visualiser (3D): User Properties & Configuration** — save and apply Wallpaper Engine user properties/configuration from `project.json`, display in KDE plugin, per-wallpaper persistence. Regression checklist: `doc/razer-visualiser-properties-checklist.md`.

### Security / Safety

- **Null-texture guard (renderer submodule):** Guard `QSGSimpleTextureNode::setTexture` against null texture in headless/offscreen GL environments. Previously, `TextureNode` called `createTextureFromGl(0, …)` as a placeholder seed; when `fromNative` returned `nullptr` (no Mesa llvmpipe, CI, Qt offscreen platform), the null pointer was passed to `setTexture` which dereferences it and crashes plasmashell with SIGSEGV. Now the node degrades to transparent with a warning instead of crashing. Commit `b79d5ae` in wallpaper-scene-renderer fork, branch `dev/null-texture-guard`.
- **SafeWallpaperBridge:** Read-only QWebChannel surface — properties are `READ`-only, no `Q_INVOKABLE` methods, grep-able audit surface (`bef5fd5`)
- **FileHelper::readFile:** Allowlist + canonical symlink resolution + 64 MiB cap (`bef5fd5`)
- **Workshop manifest:** Canonical path resolution + 1 MiB ACF size cap

### Performance

- **P-001 In-memory regression: Workshop 3242756527** — documented case of intermittent wallpaper flicker on animated gifscene.pkg (~829 MiB, 4096×2048 textures, VMA ~438 MB / RSS ~900 MB). Diagnostic signs described (render-target flicker, max-frame spikes, VMA/RSS divergence) and directions: texture/frame budget, GIF frame upload synchronization, render-target/present synchronization, fallback. No code changes — pure documentation for future Phase 2 work. See ROADMAP.md § Performance Regression Cases.
- **P-002 In-memory regression: Workshop 3156591944 (Hackercore)** — documented case of peak VRAM consumption during loading of heavy scene.pkg (~250 MiB, ~1 GiB texture allocation cap). Directions described: texture/frame budget, graceful fallback to previous frame/wallpaper, per-wallpaper FPS, VMA/RSS/frame pacing diagnostics, VRR/Adaptive Sync awareness. No code changes — documentation task for Phase 2. See ROADMAP.md § Performance Regression Cases.
- **P-003 Intermittent flicker on light scene: `deep_space` (Workshop TBD)** — documented case of intermittent wallpaper flicker on a light scene where memory pressure is not a factor (unlike P-001). Suspected directions: swapchain/surface recreation audit, presentation scheduling (FIFO vs MAILBOX), compositor (KWin) interaction, minimal reproducer. Workshop ID needs confirmation — missing from local catalog. No code changes — pure documentation. See ROADMAP.md § Performance Regression Cases > P-003.

### Documentation

- ROADMAP.md: Phase 0 completed, Phase 1 in progress with null-texture guard as first deliverable
- ROADMAP.md: Added Performance Regression Case P-003 — `deep_space` (Workshop TBD) intermittent flicker on light scene
- ROADMAP.md: Added Feature Requirements FR-001–FR-005 (per-wallpaper FPS, disableMouse, future audio-reactive/parallax flags, persistence/GUI)
- CONTRIBUTING.md: Updated for Phase 1, added renderer fork and test instructions
- CHANGELOG.md: Initial file

### Test Results (2026-09-05)

| Suite | Tests | Result |
|-------|-------|--------|
| backend_scene_tests | ~150 | ✅ 100% passed |
| scenescript_tests | ~40 | ✅ 100% passed |
| backend_scene_thread_tests | ~5 | ✅ 100% passed |

All tests pass on GCC 16.2 / x86-64 / Kali Linux with Mesa llvmpipe. No pre-existing failures in the submodule test suite.
