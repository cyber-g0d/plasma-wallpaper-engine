# Changelog — plasma-wallpaper-engine

## [unreleased] — dev/plasmashell-safety

### Planned

- **Razer Visualiser (3D): User Properties & Configuration** — сохранение и применение пользовательских свойств/конфигурации Wallpaper Engine из `project.json`, отображение в KDE-плагине, per-wallpaper persistence. Regression checklist: `doc/razer-visualiser-properties-checklist.md`.

### Security / Safety

- **Null-texture guard (renderer submodule):** Guard `QSGSimpleTextureNode::setTexture` against null texture in headless/offscreen GL environments. Previously, `TextureNode` called `createTextureFromGl(0, …)` as a placeholder seed; when `fromNative` returned `nullptr` (no Mesa llvmpipe, CI, Qt offscreen platform), the null pointer was passed to `setTexture` which dereferences it and crashes plasmashell with SIGSEGV. Now the node degrades to transparent with a warning instead of crashing. Commit `b79d5ae` in wallpaper-scene-renderer fork, branch `dev/null-texture-guard`.
- **SafeWallpaperBridge:** Read-only QWebChannel surface — properties are `READ`-only, no `Q_INVOKABLE` methods, grep-able audit surface (`bef5fd5`)
- **FileHelper::readFile:** Allowlist + canonical symlink resolution + 64 MiB cap (`bef5fd5`)
- **Workshop manifest:** Canonical path resolution + 1 MiB ACF size cap

### Performance

- **P-001 In-memory regression: Workshop 3242756527** — задокументирован случай периодического мерцания обоев на анимированном gifscene.pkg (~829 MiB, текстуры 4096×2048, VMA ~438 MB / RSS ~900 MB). Описаны диагностические признаки (render-target flicker, max-frame spikes, VMA/RSS divergence) и направления: texture/frame budget, GIF frame upload synchronization, render-target/present synchronization, fallback. Код не менялся — чистая документация для будущей работы по Phase 2. См. ROADMAP.md § Performance Regression Cases.
- **P-002 In-memory regression: Workshop 3156591944 (Hackercore)** — задокументирован случай пикового потребления VRAM при загрузке тяжёлого scene.pkg (~250 MiB, ~1 GiB texture allocation cap). Описаны направления: texture/frame budget, graceful fallback на предыдущий кадр/обои, per-wallpaper FPS, диагностика VMA/RSS/frame pacing, учёт VRR/Adaptive Sync. Код не менялся — документационная задача для Phase 2. См. ROADMAP.md § Performance Regression Cases.
- **P-003 Intermittent flicker on light scene: `deep_space` (Workshop TBD)** — задокументирован случай периодического мерцания обоев на лёгкой сцене, где memory pressure не является фактором (в отличие от P-001). Предполагаемые направления: swapchain/surface recreation audit, presentation scheduling (FIFO vs MAILBOX), compositor (KWin) interaction, minimal reproducer. Workshop ID требует подтверждения — отсутствует в локальном каталоге. Код не менялся — чистая документация. См. ROADMAP.md § Performance Regression Cases > P-003.

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
