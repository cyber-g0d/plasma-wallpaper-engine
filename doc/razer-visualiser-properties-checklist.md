# Razer Visualiser (3D) — User Properties Regression Checklist

Checklist для верификации сохранения, загрузки, применения и отображения
пользовательских свойств Wallpaper Engine для Razer Visualiser (3D).

## Источник данных

Свойства читаются из `project.json` wallpaper-а (секция `userproperties`),
сохраняются per-wallpaper и отображаются в KDE-плагине.

---

## 1. Загрузка свойств из project.json

- [ ] **1.1** `project.json` содержит секцию `userproperties` → свойства загружены
- [ ] **1.2** `project.json` без секции `userproperties` → используются значения по умолчанию (не краш)
- [ ] **1.3** `project.json` отсутствует → fallback к значениям по умолчанию (не краш)
- [ ] **1.4** `project.json` повреждён (невалидный JSON) → ошибка залогирована, fallback к defaults
- [ ] **1.5** Пустая секция `userproperties: {}` → корректно, значений нет, defaults
- [ ] **1.6** Числовые property: int, float, range → корректный парсинг и приведение типов
- [ ] **1.7** Булевы property: `true`/`false` → корректный парсинг
- [ ] **1.8** Строковые property (color hex, enum) → корректный парсинг
- [ ] **1.9** Property с `options` (dropdown) → значения валидируются против допустимых
- [ ] **1.10** Property с `min`/`max`/`step` → значения клиппятся в допустимые границы

## 2. Применение свойств к визуализатору

- [ ] **2.1** Загруженные свойства передаются в Razer Visualiser renderer
- [ ] **2.2** Изменение свойства через UI → немедленное применение к визуализатору (live preview)
- [ ] **2.3** Сброс свойства на default → визуализатор переходит к значению по умолчанию
- [ ] **2.4** Все property из `project.json` имеют соответствующий control в визуализаторе
- [ ] **2.5** Свойства, неподдерживаемые визуализатором → игнорируются с warning (не краш)

## 3. Per-wallpaper persistence

- [ ] **3.1** Изменение свойства → сохраняется для текущего wallpaper
- [ ] **3.2** Переключение на другой wallpaper → его собственные свойства загружены (не из предыдущего)
- [ ] **3.3** Возврат на предыдущий wallpaper → восстановлены его сохранённые свойства
- [ ] **3.4** Сохранение переживает перезапуск plasmashell
- [ ] **3.5** Сохранение переживает logout/login (KDE session restart)
- [ ] **3.6** Несколько wallpaper с одинаковым `project.json` → независимые сохранённые значения
- [ ] **3.7** Удаление wallpaper → сохранённые свойства очищены (не orphaned)

## 4. Отображение свойств в KDE-плагине

- [ ] **4.1** KDE-плагин показывает все property из `project.json` для текущего wallpaper
- [ ] **4.2** Типы controls соответствуют типу property:
  - `int` / `float` с `min`/`max` → слайдер
  - `bool` → checkbox / toggle
  - `color` → color picker
  - `combo` / `options` → dropdown
- [ ] **4.3** Labels/descriptions из `project.json` отображаются корректно (включая Unicode)
- [ ] **4.4** Controls отзывчивы: изменение слайдера/checkbox мгновенно отражается
- [ ] **4.5** Плагин не показывает properties для неподдерживаемого типа wallpaper (scene/web/video)
- [ ] **4.6** При отсутствии `project.json` — плагин показывает сообщение «нет настраиваемых параметров»

## 5. Edge cases

- [ ] **5.1** `userproperties` с очень большим количеством свойств (>100) → скроллится без лагов
- [ ] **5.2** Property с очень длинным именем/description → не ломает layout
- [ ] **5.3** Одновременное изменение нескольких свойств (быстрое перетаскивание слайдера) → не глючит
- [ ] **5.4** Переключение wallpaper во время изменения свойства → состояние не теряется
- [ ] **5.5** Файл сохранения повреждён → fallback к `project.json` defaults, старый файл перезаписан

## 6. Регрессионные проверки

- [ ] **6.1** Базовый функционал wallpapers не сломан (video, web, scene)
- [ ] **6.2** `backend_scene_tests` — все тесты проходят
- [ ] **6.3** `scenescript_tests` — все тесты проходят
- [ ] **6.4** `backend_scene_thread_tests` — все тесты проходят
- [ ] **6.5** Нет утечек памяти (valgrind/ASAN на property load/save цикле)
- [ ] **6.6** Нет крашей plasmashell при:
  - запуске с Razer Visualiser wallpaper
  - переключении на/с Razer Visualiser
  - изменении любого свойства через плагин

---

*Last updated: 2026-09-05*
*Related: ROADMAP.md Phase 1 — Razer Visualiser (3D): User Properties & Configuration*
