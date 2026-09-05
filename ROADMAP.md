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
| `MpvBackend` (libmpv video) | ⚠️ In-process with plasmashell — libmpv crashes take down the desktop. Mitigation: upstream has render-thread unblocking on shutdown (6fc5cea) |
| `QtWebEngine` (web wallpapers) | ⚠️ In-process with plasmashell — `--disable-web-security` flag is required for workshop compatibility. Mitigation: `WebUrlInterceptor` filters file:// requests |

## Phase 1: Defensive Hardening (in progress)

**Goal:** Prevent wallpaper bugs from crashing plasmashell.

- [x] **Null-texture guard in TextureNode** — guard QSGSimpleTextureNode::setTexture against null texture in headless/offscreen GL (commit b79d5ae, renderer fork `dev/null-texture-guard`)
- [ ] **Razer Visualiser (3D): User Properties & Configuration** — сохранять и применять Wallpaper Engine user properties/configuration из `project.json`, отображать их в KDE-плагине, обеспечить per-wallpaper persistence. См. `doc/razer-visualiser-properties-checklist.md` для regression-тестирования.
- [ ] Catch-all error handling in wallpaper loading paths
- [ ] Graceful fallback to static color/blank on wallpaper load failure
- [ ] Signal-slot safety audit — verify no cascading failures
- [ ] Thread-safety review — wallpaper enumeration runs off main thread?
- [x] Steam library enumeration hardening — handle corrupt/missing workshop data (Phase 0)
- [ ] Fuzz harness for wallpaper property parsing

## Phase 2: Resource Limits (planned)

**Goal:** Prevent wallpaper resource usage from degrading the desktop.

- [ ] FPS limit for video wallpapers (configurable, default 30)
- [ ] **Per-wallpaper FPS** — независимый лимит для scene-обоев, автоснижение при VRAM-давлении, ручная настройка через KDE-плагин; см. P-001, P-002
- [ ] VRAM budget enforcement
- [ ] **Heavy scene texture budget** — per-wallpaper лимит GPU-текстур (~256–512 MiB), downscale/streaming при превышении, предупреждение пользователю; см. P-002
- [ ] **Graceful fallback chain** — fallback на предыдущий кадр/обои при OOM/allocation failure, сохранение последнего успешного кадра, авто-retry; см. P-002
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

## Performance Regression Cases

### P-001: Intermittent wallpaper-only flicker (Workshop 3242756527)

**Status:** Open — diagnostic phase, no code changes

**Workshop ID:** 3242756527
**Wallpaper type:** animated gifscene.pkg (~829 MiB)
**Texture resolution:** 4096×2048 (per-frame)
**Memory footprint:** VMA ~438 MB, RSS ~900 MB
**Frame pacing:** mostly stable 30/24 FPS, occasional max-frame spikes

#### Диагностические признаки

| Признак | Наблюдение | Что проверять |
|---------|-----------|---------------|
| Мерцание только обоев | Панели/виджеты Plasma не затрагиваются — проблема изолирована в render target обоев | Проверить, не сбрасывается ли `VkFramebuffer`/`VkRenderPass` между кадрами; не гоняется ли `vkQueueSubmit` за acquire-present циклом |
| Привязка к анимированному gifscene.pkg | Статические сцены и видео не дают flicker | Сравнить pipeline загрузки GIF-кадров с обычным scene-рендерингом: формат, mip-уровни, тайлинг текстур |
| Текстуры 4096×2048 | ~32 MB на кадр в RGBA8; несколько таких в пуле быстро насыщают VRAM | Проверить VMA-статистику: фрагментацию, количество аллокаций, хиты в `VMA_MEMORY_USAGE_GPU_ONLY` |
| VMA ~438 MB при RSS ~900 MB | VMA учитывает только GPU-аллокации; разница ~460 MB — CPU-side копии, staging-буферы, Qt-структуры | Проверить, не дублируются ли GIF-кадры в CPU-памяти после upload на GPU |
| Max-frame spikes при стабильном average | Периодические «длинные» кадры на фоне ровного 30/24 FPS | Искать блокирующие операции на render-потоке: синхронная загрузка с диска, ожидание fence, пересоздание swapchain |

#### Будущие направления

1. **Texture / frame budget (Phase 2):**
   - Ввести лимит на суммарный размер GPU-текстур на сцену (~256–512 MB)
   - При превышении — downscale текстур или throttling частоты загрузки GIF-кадров
   - Добавить `VmaBudget`-based мониторинг с предупреждением до фактического исчерпания

2. **GIF frame upload synchronization:**
   - Проверить, не загружаются ли GIF-кадры синхронно в render-поток (блокирующий `glTexImage2D`/`vkCmdCopyBufferToImage` без стейджинга)
   - Рассмотреть асинхронный upload через dedicated transfer queue + двойную буферизацию staging-буферов
   - Профилировать `gifscene.pkg`-специфичный кодек: возможно, узкое место — декодирование, а не upload

3. **Render target / present synchronization:**
   - Аудит `vkAcquireNextImageKHR` → render → `vkQueuePresentKHR` цепочки:
     - Не вызывается ли `vkDeviceWaitIdle` между кадрами
     - Правильно ли расставлены `VkSemaphore`/`VkFence` (нет ли double-wait или missing signal)
   - Проверить режим презентации (`VK_PRESENT_MODE_FIFO` vs `MAILBOX` vs `IMMEDIATE`): flicker на FIFO может указывать на пропуск vblank из-за late submit
   - Исключить неявный `vkQueueWaitIdle` в hot path (например, внутри VMA-дефрагментации)

4. **Fallback:**
   - При детекции flicker > N кадров подряд — переход на статический кадр с градиентной заливкой
   - При исчерпании VRAM — graceful degradation: пропуск кадров, fallback-текстура низкого разрешения
   - Логирование статистики кадров в диагностический бандл (Phase 4)

#### Regression test plan

- [ ] Воспроизвести на том же Workshop ID с включённым `VK_LAYER_LUNARG_monitor`
- [ ] Захватить GPU trace (RenderDoc / `VK_LAYER_LUNARG_api_dump`) на flicker-кадре
- [ ] Сравнить GPU-тайминги между flicker-кадром и соседними стабильными
- [ ] Профилировать распределение VMA: `VmaBudget`, `VmaDetailedStatistics`
### P-002: Heavy Scene wallpaper texture exhaustion (Workshop 3156591944 — Hackercore)

**Status:** Open — diagnostic phase, no code changes

**Workshop ID:** 3156591944
**Wallpaper type:** scene.pkg (~250 MiB compressed, ~1 GiB texture allocation at peak)
**Memory footprint:** VMA allocation cap ~1 GiB during loading, RSS significantly above VMA
**Frame pacing:** TBD — initial loading stresses allocator before first stable frame

#### Диагностические признаки

| Признак | Наблюдение | Что проверять |
|---------|-----------|---------------|
| scene.pkg ~250 MiB | На порядок тяжелее типичных сцен; содержит большое количество предзагруженных текстур, шейдеров, мешей | Профилировать загрузку: какие ассеты занимают больше всего места в архиве? Есть ли неиспользуемые ассеты? |
| Texture allocation cap ~1 GiB | VMA-аллокатор достигает ~1 GiB при загрузке — близко к границе доступной VRAM на многих GPU | Мониторинг `VmaBudget` и `VmaDetailedStatistics` в процессе загрузки; определить пиковое потребление и момент стабилизации |
| Загрузка до первого кадра | Потенциально долгий startup из-за массовой загрузки ассетов | Измерить wall-clock time от `init()` до первого `render()`; сравнить с лёгкими сценами |
| RSS существенно выше VMA | Разница между RSS и VMA указывает на CPU-side дублирование или staging-буферы | Проверить, освобождаются ли staging-буферы после GPU-upload; нет ли утечек в цикле загрузки |

#### Задачи

1. **Texture / frame budget (Phase 2):**
   - Ввести per-wallpaper лимит на суммарный размер GPU-текстур (~256–512 MiB по умолчанию, с возможностью повышения для тяжёлых сцен)
   - При превышении в процессе загрузки — graceful degradation: downscale текстур, отложенная загрузка (streaming), или fallback-текстура низкого разрешения
   - Предупреждение пользователю через KDE-плагин, если сцена требует больше VRAM, чем доступно
   - Добавить `VmaBudget`-based мониторинг с предупреждением до фактического исчерпания

2. **Graceful fallback на предыдущий кадр/обои:**
   - Если wallpaper не может быть загружен (OOM, timeout, allocation failure) — fallback на предыдущие рабочие обои
   - Сохранять последний успешный кадр как статический fallback
   - Информировать пользователя через плагин о причине fallback'а
   - Автоматический retry через настраиваемый интервал

3. **Per-wallpaper FPS:**
   - Независимый FPS-лимит для scene-обоев (отдельно от video)
   - Значение по умолчанию для тяжёлых сцен — 24–30 FPS
   - Возможность ручной настройки per-wallpaper через KDE-плагин
   - Автоматическое снижение FPS при приближении к VRAM-лимиту

4. **Диагностика VMA/RSS/frame pacing:**
   - Per-wallpaper сбор статистики: VMA allocation count/size, RSS (process-wide), frame times (min/max/avg/P99), frame drops
   - Экспорт в диагностический бандл (Phase 4)
   - In-plugin индикатор здоровья: зелёный/жёлтый/красный по memory pressure и frame pacing
   - Логирование аллокаций с тегами для отслеживания утечек

5. **VRR / Adaptive Sync:**
   - Учёт VRR (Variable Refresh Rate) и Adaptive Sync при выставлении FPS-лимита
   - Автоопределение VRR-диапазона дисплея (напр. 48–144 Hz) через DRM/KMS или `VK_EXT_display_control`
   - При активном VRR — таргетировать FPS в нижней половине VRR-диапазона для минимизации LFC (Low Framerate Compensation)
   - Fallback: если VRR недоступен — использовать классический `VK_PRESENT_MODE_FIFO` с vblank-синхронизацией
   - Документирование лучших практик FPS для VRR-дисплеев

#### Regression test plan

- [ ] Загрузить Workshop 3156591944 на GPU с 2/4/8 GiB VRAM и замерить `VmaBudget` до/после загрузки
- [ ] Захватить GPU trace (RenderDoc) на момент пиковой аллокации — определить топ-10 крупнейших текстур
- [ ] Профилировать startup latency: `init()` → первый кадр → стабильный FPS
- [ ] Проверить поведение при принудительном ограничении VRAM через `VK_EXT_memory_budget` mock
- [ ] Стресс-тест: циклическая смена wallpaper на 3156591944 и обратно — проверить отсутствие утечек (VMA + RSS)

### P-003: Intermittent wallpaper-only flicker on light scene — `deep_space` (Workshop TBD)

**Status:** Open — needs Workshop ID confirmation

**Workshop ID:** TBD — отсутствует в локальном каталоге Steam Workshop. Требуется подтверждение от пользователя.
**Wallpaper nickname:** `deep_space`
**Wallpaper type:** TBD (предположительно scene или video, лёгкая сцена)
**Texture resolution:** TBD — заведомо ниже 4096×2048 (лёгкая сцена)
**Memory footprint:** TBD — ожидается значительно ниже VMA ~438 MiB / RSS ~900 MiB эталона P-001
**Frame pacing:** TBD — intermittent flicker при стабильном в среднем FPS

#### Отличие от P-001

P-001 привязан к тяжёлому gifscene.pkg (Workshop 3242756527) с текстурами 4096×2048 и VMA ~438 MiB. Фликер там объясним через VMA-фрагментацию, staging-дублирование и гонку acquire-present.

P-003 — flicker на **лёгкой сцене**, где memory pressure не должен быть фактором. Это указывает на другую природу flicker'а, не связанную с исчерпанием ресурсов:
- Возможен race condition в swapchain-recreation, не привязанный к объёму VRAM
- Возможен баг в логике presentation scheduling, не зависящий от сложности сцены
- Возможна проблема с compositor-взаимодействием (KWin) — например, неверный damage-region или partial-update, вызывающий пропуск кадра на стороне композитора

#### Диагностические признаки (предварительные)

| Признак | Ожидание для лёгкой сцены | Что проверять |
|---------|--------------------------|---------------|
| Flicker только обоев, панели стабильны | Как в P-001 — изоляция в render target | Проверить, общий ли это баг presentation pipeline, не зависящий от нагрузки |
| Лёгкая сцена (предположительно) | VMA < 100 MiB, RSS < 200 MiB | Исключить memory pressure как причину; если flicker воспроизводится — значит корневая причина не в VMA |
| Intermittent, нерегулярный | Не привязан к конкретным кадрам или фазам загрузки | Искать недерминированные факторы: системные события, композитор-режимы, vblank-тайминг |
| Стабильный средний FPS | Нет max-frame spikes, в отличие от P-001 | Искать проблему в present-path, а не render-path: `vkQueuePresentKHR`, surface capabilities, режим FIFO/MAILBOX |

#### Будущие направления

1. **Swapchain / surface recreation audit:**
   - Проверить, не пересоздаётся ли swapchain чаще необходимого (напр. на каждый `resizeEvent`, даже без изменения размера)
   - Верифицировать `VkSurfaceCapabilitiesKHR::currentExtent` — не возвращает ли композитор (0,0) или stale-значения
   - Проверить обработку `VK_ERROR_OUT_OF_DATE_KHR` и `VK_SUBOPTIMAL_KHR` — нет ли ложных срабатываний

2. **Presentation scheduling:**
   - Сравнить `VK_PRESENT_MODE_FIFO` vs `MAILBOX` на проблемной сцене — исчезает ли flicker при смене режима
   - Проверить, не вызывает ли KWin partial-update damage tracking некорректную отрисовку области обоев
   - Исключить vblank-miss из-за scheduling jitter: замерить `present_id` / `present_time` через `VK_EXT_present_timing`

3. **Compositor (KWin) interaction:**
   - Проверить damage-region: не отправляет ли плагин пустой/нулевой damage, заставляя KWin перерисовывать всю область
   - Верифицировать `wl_surface::damage_buffer` и `zwp_linux_buffer_params_v1` — корректность buffer-release цикла
   - Исключить гонку между buffer-release KWin и acquire-next-image плагина

4. **Minimal reproducer:**
   - Создать минимальную сцену (один треугольник, статическая текстура), которая даёт flicker
   - Если не даёт — значит проблема в чём-то специфичном для ассетов `deep_space`
   - Бисекция ассетов `deep_space`: отключать слои/эффекты по одному, пока flicker не исчезнет

#### Regression test plan

- [ ] Подтвердить Workshop ID `deep_space` и подписаться в Steam
- [ ] Воспроизвести flicker на лёгкой сцене с включённым `VK_LAYER_LUNARG_monitor`
- [ ] Сравнить swapchain-метрики (create/destroy count, present-timing) между flicker-сессией и стабильной
- [ ] Протестировать оба presentation mode (FIFO / MAILBOX) — записать, исчезает ли flicker
- [ ] Захватить KWin debug log (`KWIN_LOG=debug`) в момент flicker'а — проверить damage-region и buffer-release
- [ ] Создать minimal reproducer сцену для изоляции корневой причины

## Feature Requirements: Per-Wallpaper Configuration & Future Flags

### FR-001: Per-Wallpaper FPS Limit

**Приоритет:** Phase 2 (Resource Limits)

- Независимый FPS-лимит для каждого типа обоев: scene, video, web
- Значения по умолчанию:
  - Scene: 30 FPS (тяжёлые) / 60 FPS (лёгкие, автоопределение)
  - Video: 30 FPS (совпадает с типичным фреймрейтом видео)
  - Web: 15 FPS (щадящий режим для QtWebEngine)
- Возможность ручной настройки per-wallpaper через KDE-плагин
- Автоматическое снижение FPS при приближении к VRAM-лимиту (throttling)
- Учёт VRR/Adaptive Sync: автоопределение VRR-диапазона, таргетинг в нижней половине диапазона для экономии энергии

### FR-002: disableMouse Flag

**Приоритет:** Phase 2 (Resource Limits)

- Per-wallpaper флаг, отключающий обработку событий мыши для обоев (движение, клики, drag)
- Мотивация:
  - Снижение CPU-нагрузки от ненужной обработки событий (особенно для scene-обоев с particle-эффектами, реагирующими на мышь)
  - Устранение потенциального источника jitter/flicker: обработка mouse-event в render-потоке может вызывать микро-задержки
  - Экономия энергии на ноутбуках (пробуждение CPU на каждое движение мыши)
- По умолчанию: `false` (мышь включена) для обратной совместимости
- GUI: чекбокс в KDE-плагине, per-wallpaper

### FR-003: Future Audio-Reactive Flags

**Приоритет:** Phase 3+ (Process Isolation)

- Per-wallpaper флаги для аудио-реактивных обоев (визуализация спектра, waveform, audio-driven particles)
- Планируемые флаги:
  - `audioReactive` (bool) — включает/отключает audio pipeline для обоев
  - `audioSource` (enum: `system`, `microphone`, `application`) — источник аудио
  - `audioSensitivity` (float 0.0–2.0) — множитель чувствительности
  - `audioSmoothing` (float 0.0–1.0) — сглаживание амплитуд (экспоненциальное скользящее среднее)
  - `audioBands` (int 8–256) — количество частотных полос для FFT
- Требования:
  - Аудио-захват должен работать в изолированном процессе (Phase 3), чтобы не блокировать plasmashell
  - Низкая latency (< 16 ms) через PulseAudio/PipeWire real-time scheduling
  - Автоматическое отключение audio pipeline при `disableMouse` + отсутствии других потребителей — экономия CPU

### FR-004: Future Parallax Flags

**Приоритет:** Phase 3+ (Process Isolation)

- Per-wallpaper флаги для параллакс-эффекта (смещение слоёв относительно движения мыши / акселерометра)
- Планируемые флаги:
  - `parallaxEnabled` (bool) — включает параллакс
  - `parallaxStrength` (float 0.0–2.0) — сила эффекта
  - `parallaxMode` (enum: `mouse`, `accelerometer`, `both`) — источник движения
  - `parallaxLayers` (int 2–10) — количество слоёв для depth-based смещения
- Требования:
  - Параллакс должен учитывать `disableMouse`: если мышь отключена, parallax через `mouse` недоступен
  - Аппаратное ускорение через GPU (vertex-shader displacement), без CPU-side обработки
  - Совместимость с VRR: плавное смещение без разрывов на любом FPS

### FR-005: Per-Wallpaper Persistence & GUI

**Приоритет:** Phase 1 (Defensive Hardening) — базовая версия; Phase 4 — расширенная

- Сохранение per-wallpaper настроек (FPS, disableMouse, audio, parallax) в локальном конфиге (JSON/QSettings)
- Структура конфига:
  ```json
  {
    "workshopId": "TBD",
    "fpsLimit": 30,
    "disableMouse": false,
    "audioReactive": false,
    "parallaxEnabled": false
  }
  ```
- Путь: `~/.config/wallpaper-engine-kde/workshop/<workshopId>.json`
- Миграция: при обновлении плагина — автоматический перенос старых настроек в новую схему
- GUI (KDE-плагин):
  - Вкладка «Wallpaper Settings» в окне настройки обоев
  - Элементы: FPS slider (1–120), disableMouse checkbox
  - Расширенные настройки (audio, parallax) — в collapsible-секции «Advanced»
  - Индикатор здоровья обоев (Phase 4): FPS, VRAM, crash count — overlay в preview
  - Кнопка «Reset to Defaults» для сброса per-wallpaper настроек
- Persistence между сессиями plasmashell: настройки загружаются при старте wallpaper, сохраняются при изменении
- Валидация: при загрузке конфига — проверка границ значений, fallback на defaults при повреждённом JSON



---

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md). During Phase 1, contributions are welcome for:
- Defensive hardening patches (null guards, error recovery, fallbacks)
- Reproducible crash test cases
- Architecture discussions for Phase2–4

Safety-critical patches land in `dev/plasmashell-safety`; renderer patches land in `dev/null-texture-guard` (renderer fork).

---

*Last updated: 2026-09-05 (P-002, P-003 added; FR-001–FR-005)*
*Maintainer: [cyber-g0d](https://github.com/cyber-g0d)*
