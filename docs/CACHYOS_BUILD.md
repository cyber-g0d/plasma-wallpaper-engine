# CachyOS build verification

Verified working on CachyOS / KDE Plasma 6:

- CachyOS, plasmashell 6.7.5, Qt 6.11.2, KF6 6.30, GCC 16.2.1 / Clang 22.1.8
- cmake 4.4.3, ninja 1.13.2, Vulkan 1.4.357 (HW video texture decoder enabled)
- cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr -G Ninja
- cmake --build build  -> exit 0, builds src/libWallpaperEngineKde.so
- sudo cmake --install build -> plugin loads and runs in a live Plasma session

Scope note: verified on fork head 89e5b01f; upstream CaptSilver may be ahead.
