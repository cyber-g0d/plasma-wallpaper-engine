# Roadmap — plasma-wallpaper-engine

## Phase 0: Foundation (current)

**Goal:** Understand the codebase and establish safety baselines.

- [ ] **Source review: `plugin/`** — KDE Plasma 6 plugin interface, plasmashell lifecycle hooks
- [ ] **Source review: `src/`** — wallpaper enumeration, loading, type dispatch (scene/web/video)
- [ ] **Source review: `src/backend_scene/`** — Vulkan renderer, resource lifecycle, error paths
- [ ] **Reproducer: plasmashell crash on invalid wallpaper** — minimal corrupt/empty asset that crashes plasmashell
- [ ] **Reproducer: resource exhaustion** — wallpaper that leaks VRAM or file descriptors over time
- [ ] **Baseline test suite** — ensure existing tests pass, identify gaps

## Phase 1: Defensive Hardening (planned)

**Goal:** Prevent wallpaper bugs from crashing plasmashell.

- [ ] Catch-all error handling in wallpaper loading paths
- [ ] Graceful fallback to static color/blank on wallpaper load failure
- [ ] Signal-slot safety audit — verify no cascading failures
- [ ] Thread-safety review — wallpaper enumeration runs off main thread?
- [ ] Steam library enumeration hardening — handle corrupt/missing workshop data
- [ ] Fuzz harness for wallpaper property parsing

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

## Phase 4: Observability (planned)

**Goal:** Make wallpaper health visible and debuggable.

- [ ] Per-wallpaper health metrics (FPS, VRAM, crash count, load time)
- [ ] Diagnostic bundle — collect logs, backtraces, system info for bug reports
- [ ] Plasma widget for wallpaper health overlay

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md). During Phase 0, contributions are limited to:
- Source review findings
- Reproducible crash test cases
- Architecture discussions

---

*Last updated: 2026-09-05*
*Maintainer: [cyber-g0d](https://github.com/cyber-g0d)*
