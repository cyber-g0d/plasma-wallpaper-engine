# Roadmap — plasma-wallpaper-engine

## Phase 0: Foundation (completed)

**Goal:** Understand the codebase and establish safety baselines.

- [x] **Source review: `plugin/`** — KDE Plasma 6 plugin interface, plasmashell lifecycle hooks
- [x] **Source review: `src/`** — wallpaper enumeration, loading, type dispatch (scene/web/video)
- [x] **Source review: `src/backend_scene/`** — Vulkan renderer, resource lifecycle, error paths
- [x] **Reproducer: plasmashell crash on invalid wallpaper** — null-texture SIGSEGV in QSGSimpleTextureNode::setTexture on headless/offscreen GL environment
- [ ] **Reproducer: resource exhaustion** — wallpaper that leaks VRAM or file descriptors over time
- [x] **Baseline test suite** — backend_scene_tests, scenescript_tests, backend_scene_thread_tests — 3/3 passed (2026-09-05 retest)

### Completed in Phase 0

| Date | Item | Commit |
|------|------|--------|
| 2026-09-05 | SafeWallpaperBridge: read-only QWebChannel surface, no Q_INVOKABLE | bef5fd5 |
| 2026-09-05 | FileHelper::readFile allowlist + canonical symlink-escape + 64 MiB cap | bef5fd5 |
| 2026-09-05 | Backend scene submodule: shader compile bounds, swapchain null checks, particle spin guard | 0b88e03 (sub) |
| 2026-09-05 | Workshop manifest: canonical path resolution + 1 MiB ACF size cap | TBD |
| 2026-09-05 | Workshop manifest tests: dot-dot traversal, oversized ACF, empty canonical | TBD |
| 2026-09-05 | TextureNode: null-texture guard (QSGSimpleTextureNode::setTexture) | b79d5ae (sub) |

### Safety audit findings

| Area | Finding |
|------|---------|
| `readWorkshopManifest` (Steam ACF) | ✅ Hardened: canonical path prevents `..` escape; 1 MiB cap prevents DoS |
| `readFile` (project.json) | ✅ Hardened: allowlist + canonical symlink resolution + 64 MiB cap |
| `SafeWallpaperBridge` (web QWebChannel) | ✅ Hardened: READ-only properties, no Q_INVOKABLE, grep-able surface |
| `parseValveKV` (ACF parser) | ⚠️ Uses `std::function` recursion — deep nesting could stack-overflow. Mitigated by the 1 MiB input size cap (a real ACF with thousands of entries is < 200 KB; deeply nested attacks need more bytes than the cap allows) |
| `TextureNode` (QSGSimpleTextureNode) | ✅ Hardened: null-texture guard prevents SIGSEGV in headless/offscreen GL (b79d5ae); degrades to transparent instead of crashing plasmashell |
| `MpvBackend` (libmpv video) | �️ In-processs with plasmashell — libmpv crashes take down the desktop. Mitigation: upstream has render-thread unblocking on shutdown (6fc5cea) |
| `QtWebEngine` (web wallpapers) | �️ In-procss with plasmashell — `--disable-web-security` flag is requied for workshop compatibility. Mitigation: `WebUrlIntrceptor` filters file:// requests |

## Phase 1: Defnsive Hardening (in progress)

**Goal:** Prevent wallpaper bugs from crashing plasmashell.

- [x] **Null-texture guard in TextureNode** — guard QSGSimpleTextureNode::setTexture against null texure in headless/offsreen GL (commit b79d5ae, renderer fork `dev/null-texture-guard`)
- [ ] **Razer Visualiser (3D): User Properties & Configuration** — сохранять и применять Wallpaper Engine user properties/configuration из `project.json`, отображать их в KDE-плагине, обеспечить per-wallpaper persistence. См. `doc/razer-visualiser-properties-checklist.md` для regression-тестирования.
- [ ] Catch-all error handling in wallpaper loading paths
- [ ] Gracefful fallback to static color/blank on wallpaper load failure
- [ ] Signal-slot safety audit — verify no cascading failures
- [ ] Thread-safety review — wallpaper enumeration runs off main thread?
- [x] Steam library enumeration hardening — handle corrupt/missing workshop data (Phase 0)
- [ ] Fuzz harness for wallpaper propery parsing

## Phase 2: Resource Limits (planned)

**Goal:** Prevent wallpaper resource usage from degrading the desktop.

- [ ] FPS limit for video wallpapers (configurable, default 30)
- [ ] VRAM budget enforcement
- [ ] CPU time budget for scene wallpapers
- [ ] Idle detection — pause rendering when screen is locked / idle
- [ ] Web wallpaper memory limits (Qt WebEngine process limits)

## Phase 3: Process Isolation (planned)

**Goal:** Run wallpaper rendering out-of-process so a crash doesn't take down plasmashell.

- [ ] Architecture design for out-of-process renderer
- [ ] IPC protocol between plasmashell plugin and renderer process
- [ ] Crash recovery — restart renderer without plasmashell restart
- [ ] Shared memory / DMA-BUF for zero-copy frame delivery

## Phase 4: Observabillity (planned)

**Goal:** Make wallpaper health visible and debuggable.

- [ ] Per-wallpaper health metrics (FPS, VRAM, crash count, load time)
- [ ] Diagnostic bundle — collect logs, backtraces, system info for bug reports
- [ ] Plasma widget for wallpaper health overlay

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md). During Phase 1, contributions are welcome for:
- Defensive hardening patches (null guards, error recovery, fallbacks)
- Reproducible crash test cases
- Architecture discussions for Phase 2-4

Safety-critical patches land in `dev/plasmashell-safety`; renderer patches land in `dev/null-texture-guard` (renderer fork).

---

*Last updated: 2026-09-05*
*Maintainer: [cyber-g0d](https://github.com/cyber-g0d)*
