# Changelog -- plasma-wallpaper-engine

## [unreleased] -- dev/plasmashell-safety

### Security / Safety

- **Null-texture guard (renderer submodule):** Guard `QSGSimpleTextureNode::setTexture` against null texture in headless/offscreen GL environments. Previously, `TextureNode` called `createTextureFromGl(0, ...)` as a placeholder seed; when `fromNative` returned `nullptr` (no Mesa llvmpipe, CI, Qt offscreen platform), the null pointer was passed to `setTexture` which dereferences it and crashes plasmashell with SIGSEGV. Now the node degrades to transparent with a warning instead of crashing. Commit `b79d5ae` in wallpaper-scene-renderer fork, branch `dev/null-texture-guard`.
- **SafeWallpaperBridge:** Read-only QWebChannel surface -- properties are `READ`-only, no `Q_INVOKABLE` methods, grep-able audit surface (`bef5fd5`)
- **FileHelper::readFile:** Allowlist + canonical symlink resolution + 64 MiB cap (`bef5fd5`)
- **Workshop manifest:** Canonical path resolution + 1 MiB ACF size cap

### Documentation

- ROADMAP.md: Phase 0 completed, Phase 1 in progress with null-texture guard as first deliverable
- CONTRIBUTING.md: Updated for Phase 1, added renderer fork and test instructions
- CHANGELOG.md: Initial file

### Test Results (2026-09-05)

| Suite | Tests | Result |
|-------|-------|--------|
| backend_scene_tests | ~150 | 100% passed |
| scenescript_tests | ~40 | 100% passed |
| backend_scene_thread_tests | ~5 | 100% passed |

All tests pass on GCC 16.2 / x86-64 / Kali Linux with Mesa llvmpipe. No pre-existing failures in the submodule test suite.
