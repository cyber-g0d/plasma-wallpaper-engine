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
- [ ] **Heavy scene texture budget** — per-wallpaper GPU texture limit (~256–512 MiB), downscale/streaming on exceed, user warning; see P-002
- [ ] **Graceful fallback chain** — fallback to previous frame/wallpaper on OOM/allocation failure, keep last successful frame, auto-retry; see P-002
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

## Performance Regression Cases

### P-001: Intermittent wallpaper-only flicker (Workshop 3242756527)

**Status:** Open — diagnostic phase, no code changes

**Workshop ID:** 3242756527
**Wallpaper type:** animated gifscene.pkg (~829 MiB)
**Texture resolution:** 4096x2048 (per-frame)
**Memory footprint:** VMA ~438 MB, RSS ~900 MB
**Frame pacing:** mostly stable 30/24 FPS, occasional max-frame spikes

#### Diagnostic Signs

| Sign | Observation | What to Check |
|---------|-----------|---------------|
| Wallpaper-only flicker | Plasma panels/widgets unaffected — problem isolated to wallpaper render target | Check whether `VkFramebuffer`/`VkRenderPass` is reset between frames; whether `vkQueueSubmit` races with the acquire-present cycle |
| Tied to animated gifscene.pkg | Static scenes and video wallpapers do not flicker | Compare GIF frame upload pipeline with normal scene rendering: format, mip levels, texture tiling |
| 4096x2048 textures | ~32 MB per frame in RGBA8; several in pool quickly saturate VRAM | Check VMA stats: fragmentation, allocation count, hits in `VMA_MEMORY_USAGE_GPU_ONLY` |
| VMA ~438 MB / RSS ~900 MB | VMA tracks GPU allocations only; ~460 MB gap is CPU-side copies, staging buffers, Qt structures | Check whether GIF frames are duplicated in CPU memory after GPU upload |
| Max-frame spikes with stable average | Periodic long frames against steady 30/24 FPS baseline | Look for blocking ops on render thread: synchronous disk load, fence wait, swapchain recreation |

#### Future Directions

1. **Texture / frame budget (Phase 2):**
   - Introduce a total GPU texture size limit per scene (~256–512 MB)
   - On exceed — downscale textures or throttle GIF frame upload rate
   - Add `VmaBudget`-based monitoring with warning before actual exhaustion

2. **GIF frame upload synchronization:**
   - Check whether GIF frames are loaded synchronously on the render thread (blocking `glTexImage2D`/`vkCmdCopyBufferToImage` without staging)
   - Consider async upload via dedicated transfer queue + double-buffered staging buffers
   - Profile `gifscene.pkg`-specific codec: bottleneck may be decode, not upload

3. **Render target / present synchronization:**
   - Audit `vkAcquireNextImageKHR` -> render -> `vkQueuePresentKHR` chain:
     - Is `vkDeviceWaitIdle` called between frames?
     - Are `VkSemaphore`/`VkFence` correctly placed (no double-wait or missing signal)?
   - Check presentation mode (`VK_PRESENT_MODE_FIFO` vs `MAILBOX` vs `IMMEDIATE`): FIFO flicker may indicate vblank miss from late submit
   - Rule out implicit `vkQueueWaitIdle` in hot path (e.g. inside VMA defragmentation)

4. **Fallback:**
   - On flicker detected > N consecutive frames — switch to static frame with gradient fill
   - On VRAM exhaustion — graceful degradation: frame skip, low-res fallback texture
   - Log frame statistics to diagnostic bundle (Phase 4)

#### Regression test plan

- [ ] Reproduce on same Workshop ID with `VK_LAYER_LUNARG_monitor` enabled
- [ ] Capture GPU trace (RenderDoc / `VK_LAYER_LUNARG_api_dump`) on flicker frame
- [ ] Compare GPU timings between flicker frame and neighboring stable frames
- [ ] Profile VMA distribution: `VmaBudget`, `VmaDetailedStatistics`

### P-002: Heavy Scene wallpaper texture exhaustion (Workshop 3156591944 — Hackercore)

**Status:** Open — diagnostic phase, no code changes

**Workshop ID:** 3156591944
**Wallpaper type:** scene.pkg (~250 MiB compressed, ~1 GiB texture allocation at peak)
**Memory footprint:** VMA allocation cap ~1 GiB during loading, RSS significantly above VMA
**Frame pacing:** TBD — initial loading stresses allocator before first stable frame

#### Diagnostic Signs

| Sign | Observation | What to Check |
|---------|-----------|---------------|
| scene.pkg ~250 MiB | An order of magnitude heavier than typical scenes; contains many preloaded textures, shaders, meshes | Profile loading: which assets take the most space in the archive? Are there unused assets? |
| Texture allocation cap ~1 GiB | VMA allocator reaches ~1 GiB during load — near available VRAM boundary on many GPUs | Monitor `VmaBudget` and `VmaDetailedStatistics` during loading; determine peak consumption and stabilization point |
| Loading before first frame | Potentially long startup due to bulk asset loading | Measure wall-clock time from `init()` to first `render()`; compare with light scenes |
| RSS significantly above VMA | RSS-VMA gap indicates CPU-side duplication or staging buffers | Check whether staging buffers are freed after GPU upload; verify no leaks in load loop |

#### Tasks

1. **Texture / frame budget (Phase 2):**
   - Introduce per-wallpaper GPU texture size limit (~256–512 MiB default, raisable for heavy scenes)
   - On exceed during load — graceful degradation: texture downscale, deferred loading (streaming), or low-res fallback texture
   - User warning via KDE plugin if scene requires more VRAM than available
   - Add `VmaBudget`-based monitoring with warning before actual exhaustion

2. **Graceful fallback to previous frame/wallpaper:**
   - If wallpaper cannot be loaded (OOM, timeout, allocation failure) — fallback to previous working wallpaper
   - Keep last successful frame as static fallback
   - Inform user via plugin about fallback reason
   - Auto-retry at configurable interval

3. **Per-wallpaper FPS:**
   - Independent FPS limit for scene wallpapers (separate from video)
   - Default for heavy scenes — 24–30 FPS
   - Manual per-wallpaper tuning via KDE plugin
   - Auto-reduction as VRAM limit approaches

4. **VMA/RSS/frame pacing diagnostics:**
   - Per-wallpaper stats collection: VMA allocation count/size, RSS (process-wide), frame times (min/max/avg/P99), frame drops
   - Export to diagnostic bundle (Phase 4)
   - In-plugin health indicator: green/yellow/red by memory pressure and frame pacing
   - Tagged allocation logging for leak tracking

5. **VRR / Adaptive Sync:**
   - Account for VRR (Variable Refresh Rate) and Adaptive Sync when setting FPS limit
   - Auto-detect display VRR range (e.g. 48–144 Hz) via DRM/KMS or `VK_EXT_display_control`
   - With active VRR — target FPS in lower half of VRR range to minimize LFC (Low Framerate Compensation)
   - Fallback: if VRR unavailable — use classic `VK_PRESENT_MODE_FIFO` with vblank sync
   - Document FPS best practices for VRR displays

#### Regression test plan

- [ ] Load Workshop 3156591944 on GPU with 2/4/8 GiB VRAM and measure `VmaBudget` before/after load
- [ ] Capture GPU trace (RenderDoc) at peak allocation — identify top-10 largest textures
- [ ] Profile startup latency: `init()` -> first frame -> stable FPS
- [ ] Verify behavior under forced VRAM limit via `VK_EXT_memory_budget` mock
- [ ] Stress-test: cycle wallpaper to 3156591944 and back — verify no leaks (VMA + RSS)

### P-003: Intermittent wallpaper-only flicker on light scene — `deep_space` (Workshop TBD)

**Status:** Open — needs Workshop ID confirmation

**Workshop ID:** TBD — missing from local Steam Workshop catalog. Needs user confirmation.
**Wallpaper nickname:** `deep_space`
**Wallpaper type:** TBD (likely scene or video, light scene)
**Texture resolution:** TBD — known to be below 4096x2048 (light scene)
**Memory footprint:** TBD — expected significantly below the P-001 baseline of VMA ~438 MiB / RSS ~900 MiB
**Frame pacing:** TBD — intermittent flicker with stable average FPS

#### Difference from P-001

P-001 is tied to a heavy gifscene.pkg (Workshop 3242756527) with 4096x2048 textures and VMA ~438 MiB. Flicker there is explainable via VMA fragmentation, staging duplication, and acquire-present race.

P-003 — flicker on a **light scene** where memory pressure should not be a factor. This points to a different root cause, unrelated to resource exhaustion:
- Possible swapchain-recreation race condition, independent of VRAM volume
- Possible bug in presentation scheduling logic, independent of scene complexity
- Possible compositor interaction issue (KWin) — e.g. incorrect damage-region or partial-update causing frame drop on the compositor side

#### Diagnostic Signs (preliminary)

| Sign | Expectation for Light Scene | What to Check |
|---------|--------------------------|---------------|
| Wallpaper-only flicker, panels stable | Same as P-001 — isolated to render target | Check whether this is a general presentation pipeline bug, not load-dependent |
| Light scene (presumed) | VMA < 100 MiB, RSS < 200 MiB | Rule out memory pressure as cause; if flicker reproduces — root cause is not VMA |
| Intermittent, irregular | Not tied to specific frames or load phases | Look for non-deterministic factors: system events, compositor modes, vblank timing |
| Stable average FPS | No max-frame spikes, unlike P-001 | Investigate present-path, not render-path: `vkQueuePresentKHR`, surface capabilities, FIFO/MAILBOX mode |

#### Future Directions

1. **Swapchain / surface recreation audit:**
   - Check whether swapchain is recreated more often than necessary (e.g. on every `resizeEvent`, even without size change)
   - Verify `VkSurfaceCapabilitiesKHR::currentExtent` — does the compositor return (0,0) or stale values?
   - Check handling of `VK_ERROR_OUT_OF_DATE_KHR` and `VK_SUBOPTIMAL_KHR` — are there false positives?

2. **Presentation scheduling:**
   - Compare `VK_PRESENT_MODE_FIFO` vs `MAILBOX` on the problematic scene — does flicker disappear with mode change?
   - Check whether KWin partial-update damage tracking causes incorrect wallpaper region rendering
   - Rule out vblank-miss from scheduling jitter: measure `present_id` / `present_time` via `VK_EXT_present_timing`

3. **Compositor (KWin) interaction:**
   - Check damage-region: does the plugin send empty/null damage, forcing KWin to repaint the entire area?
   - Verify `wl_surface::damage_buffer` and `zwp_linux_buffer_params_v1` — correctness of buffer-release cycle
   - Rule out race between KWin buffer-release and plugin acquire-next-image

4. **Minimal reproducer:**
   - Create a minimal scene (single triangle, static texture) that produces flicker
   - If it doesn't — the problem is specific to `deep_space` assets
   - Bisect `deep_space` assets: disable layers/effects one by one until flicker disappears

#### Regression test plan

- [ ] Confirm Workshop ID for `deep_space` and subscribe on Steam
- [ ] Reproduce flicker on light scene with `VK_LAYER_LUNARG_monitor` enabled
- [ ] Compare swapchain metrics (create/destroy count, present-timing) between flicker session and stable
- [ ] Test both presentation modes (FIFO / MAILBOX) — record whether flicker disappears
- [ ] Capture KWin debug log (`KWIN_LOG=debug`) at flicker moment — check damage-region and buffer-release
- [ ] Create minimal reproducer scene to isolate root cause

## Feature Requirements: Per-Wallpaper Configuration & Future Flags

### FR-001: Per-Wallpaper FPS Limit

**Priority:** Phase 2 (Resource Limits)

- Independent FPS limit for each wallpaper type: scene, video, web
- Default values:
  - Scene: 30 FPS (heavy) / 60 FPS (light, auto-detected)
  - Video: 30 FPS (matches typical video framerate)
  - Web: 15 FPS (conservative mode for QtWebEngine)
- Manual per-wallpaper tuning via KDE plugin
- Automatic FPS reduction as VRAM limit approaches (throttling)
- VRR/Adaptive Sync aware: auto-detect VRR range, target lower half of range for energy savings

### FR-002: disableMouse Flag

**Priority:** Phase 2 (Resource Limits)

- Per-wallpaper flag that disables mouse event processing for wallpapers (move, click, drag)
- Motivation:
  - Reduce CPU load from unnecessary event processing (especially for scene wallpapers with mouse-reactive particle effects)
  - Eliminate potential jitter/flicker source: mouse-event processing on render thread can cause micro-delays
  - Power saving on laptops (CPU wakeup on every mouse movement)
- Default: `false` (mouse enabled) for backward compatibility
- GUI: checkbox in KDE plugin, per-wallpaper

### FR-003: Future Audio-Reactive Flags

**Priority:** Phase 3+ (Process Isolation)

- Per-wallpaper flags for audio-reactive wallpapers (spectrum visualization, waveform, audio-driven particles)
- Planned flags:
  - `audioReactive` (bool) — enable/disable audio pipeline for wallpaper
  - `audioSource` (enum: `system`, `microphone`, `application`) — audio source
  - `audioSensitivity` (float 0.0–2.0) — sensitivity multiplier
  - `audioSmoothing` (float 0.0–1.0) — amplitude smoothing (exponential moving average)
  - `audioBands` (int 8–256) — number of frequency bands for FFT
- Requirements:
  - Audio capture must run in an isolated process (Phase 3) to avoid blocking plasmashell
  - Low latency (< 16 ms) via PulseAudio/PipeWire real-time scheduling
  - Auto-disable audio pipeline when `disableMouse` + no other consumers — CPU savings

### FR-004: Future Parallax Flags

**Priority:** Phase 3+ (Process Isolation)

- Per-wallpaper flags for parallax effect (layer displacement relative to mouse / accelerometer movement)
- Planned flags:
  - `parallaxEnabled` (bool) — enable parallax
  - `parallaxStrength` (float 0.0–2.0) — effect strength
  - `parallaxMode` (enum: `mouse`, `accelerometer`, `both`) — motion source
  - `parallaxLayers` (int 2–10) — number of layers for depth-based displacement
- Requirements:
  - Parallax must respect `disableMouse`: if mouse is disabled, parallax via `mouse` is unavailable
  - Hardware-accelerated via GPU (vertex-shader displacement), no CPU-side processing
  - VRR compatible: smooth displacement without tearing at any FPS

### FR-005: Per-Wallpaper Persistence & GUI

**Priority:** Phase 1 (Defensive Hardening) — basic version; Phase 4 — extended

- Save per-wallpaper settings (FPS, disableMouse, audio, parallax) in local config (JSON/QSettings)
- Config structure:
  ```json
  {
    "workshopId": "TBD",
    "fpsLimit": 30,
    "disableMouse": false,
    "audioReactive": false,
    "parallaxEnabled": false
  }
  ```
- Path: `~/.config/wallpaper-engine-kde/workshop/<workshopId>.json`
- Migration: on plugin upgrade — automatic migration of old settings to new schema
- GUI (KDE plugin):
  - "Wallpaper Settings" tab in wallpaper configuration dialog
  - Controls: FPS slider (1–120), disableMouse checkbox
  - Advanced settings (audio, parallax) — in collapsible "Advanced" section
  - Wallpaper health indicator (Phase 4): FPS, VRAM, crash count — overlay in preview
  - "Reset to Defaults" button for per-wallpaper settings reset
- Persistence across plasmashell sessions: settings loaded at wallpaper start, saved on change
- Validation: on config load — bounds checking, fallback to defaults on corrupt JSON

---

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md). During Phase 1, contributions are welcome for:
- Defensive hardening patches (null guards, error recovery, fallbacks)
- Reproducible crash test cases
- Architecture discussions for Phase 2–4

Safety-critical patches land in `dev/plasmashell-safety`; renderer patches land in `dev/null-texture-guard` (renderer fork).

---

*Last updated: 2026-09-05 (P-002, P-003 added; FR-001–FR-005)*
*Maintainer: [cyber-g0d](https://github.com/cyber-g0d)*
