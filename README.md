# Лабораторная работа РБПО — статический анализ безопасности (SAST)

Учебное приложение **textzip**: сжатие текста через zlib. Сгенерировано LLM
**Qwen-Coder** (промпт и ответ — в папке `prompt/`).

- `vulnerable/` — дефектная версия: **CWE-120** (переполнение буфера через `strcpy`)
  и **CWE-78** (инъекция команд через `std::system`), зависимость **zlib 1.2.11**
  (CVE-2018-25032, проверено по NVD).
- `fixed/` — исправленная версия: безопасная обработка строки, `std::system` удалён,
  zlib обновлён до **1.3.1**.
- `reports/` — версии инструментов, команды, логи трёх SAST до/после исправления,
  разбор находок (TP/FP), сравнение, OWASP/БДУ ФСТЭК.

---

## 0. Инструкция для «нулевой» Windows-системы (установлен только VS Code)

По умолчанию в чистой Windows **нет ни компилятора C++, ни CMake** — сам VS Code их
не содержит. Выполните шаги 1–4 один раз, дальше всё запускается из VS Code.

### Шаг 1. Установить компилятор C++ и CMake (один раз)

Самый простой путь — через встроенный терминал VS Code:

1. Откройте VS Code → меню **Terminal → New Terminal** (Ctrl+`).
2. В открывшемся PowerShell выполните по очереди:

```powershell
winget install Kitware.CMake --accept-source-agreements --accept-package-agreements
winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" --accept-source-agreements --accept-package-agreements
```

Первая команда ставит CMake, вторая — Visual Studio Build Tools с MSVC-компилятором
и CMake-инструментами (workload `VCTools`, `--includeRecommended` подтягивает
Windows SDK). Установка Build Tools занимает 10–20 минут, идёт в фоне без окон.

Если `winget` недоступен (старая Windows 10): скачайте установщик
[Visual Studio Build Tools 2022](https://visualstudio.microsoft.com/ru/downloads/) ,
при установке отметите **«Desktop development with C++»**, отдельно установите
[CMake](https://cmake.org/download/) (галочка «Add CMake to PATH»).

3. **Закройте VS Code полностью и откройте заново** — чтобы обновился PATH.

Проверка (в новом терминале VS Code):

```powershell
cmake --version        # должно вывести cmake version 3.x
where.exe cl           # может не найти — это нормально, см. шаг 2
```

Компилятор `cl` живёт только внутри «Developer environment», поэтому в обычной
PowerShell его нет — и не нужно: сборку будет настраивать CMake сам.

### Шаг 2. Установить расширения VS Code

VS Code при открытии этой папки предложит установить рекомендованные расширения
(файл `.vscode/extensions.json`) — нажмите **Install**. Либо вручную (Ctrl+Shift+X):

- **C/C++** (Microsoft) — IntelliSense + отладчик;
- **CMake Tools** (Microsoft) — настройка и сборка проектов CMake.

### Шаг 3. Открыть проект и собрать

1. **File → Open Folder** → выберите папку репозитория (`lab`).
2. Откройте терминал VS Code (Ctrl+`) и выполните:

```powershell
cmake -S vulnerable -B build/vulnerable -G "Visual Studio 17 2022" -A x64
cmake --build build/vulnerable --config Release

cmake -S fixed -B build/fixed -G "Visual Studio 17 2022" -A x64
cmake --build build/fixed --config Release
```

Альтернатива без терминала: **Ctrl+Shift+B** → задача *«CMake: configure vulnerable»*,
затем *«CMake: build vulnerable»* (или через статус-бар CMake Tools: Select Kit →
«Visual Studio Build Tools 2022 Release amd64», Configure All, Build).

После сборки появятся:
`build\vulnerable\Release\textzip.exe` и `build\fixed\Release\textzip.exe`.

### Шаг 4. Запуск приложения

Вариант A — терминал VS Code:

```powershell
.\build\vulnerable\Release\textzip.exe samples\sample.txt out_vulnerable.z
"hello stdin" | .\build\vulnerable\Release\textzip.exe - out_stdin.z

.\build\fixed\Release\textzip.exe samples\sample.txt out_fixed.z
"hello fixed" | .\build\fixed\Release\textzip.exe - out_fixed_stdin.z
```

Ожидаемый вывод (vulnerable): первая строка как «comment», размеры до/после
сжатия, затем содержимое файла (это печатает уязвимый `system("type ...")`).

Вариант B — отладчик: **F5** → выберите конфигурацию
(«textzip: vulnerable (file input)», «textzip: vulnerable (stdin)», аналогично fixed).
Сборка выполнится автоматически перед запуском.

### Шаг 5. SAST-анализаторы (для отчётов)

Установка:

```powershell
winget install Cppcheck.Cppcheck --accept-source-agreements --accept-package-agreements
winget install Python.Python.3.12 --accept-source-agreements --accept-package-agreements
python -m pip install --upgrade flawfinder
```

Перезапустите VS Code. Запуск анализов — задачи VS Code:
**Terminal → Run Task…**:

- `SAST (vulnerable): cppcheck` / `MSVC /analyze` / `flawfinder`
- `SAST (fixed): cppcheck` / `MSVC /analyze` / `flawfinder`

Задача `MSVC /analyze` требует Developer PowerShell: если `cl` не найден, откройте
«Developer PowerShell for VS 2022» (в поиске Windows) и выполните команду оттуда —
она также приведена в `reports/commands.md`. Готовые отчёты лежат в `reports/`.

---

## Типовые ошибки

| Симптом | Причина и решение |
|---------|-------------------|
| `cmake : Имя "cmake" не распознано` | CMake не установлен или VS Code не перезапущен после установки. Шаг 1 + полный перезапуск VS Code. |
| `Generator 'Visual Studio 17 2022' could not find any instance of Visual Studio` | Не установлены Build Tools/VS 2022 (шаг 1) либо установлена другая версия — замените генератор, например `"Visual Studio 18 2026"` или уберите `-G` вовсе (CMake выберет сам). |
| `Does not match the generator used previously` | В `build/` остался кэш другого генератора: `Remove-Item -Recurse -Force build` и повторить конфигурацию. |
| `fatal error: zlib.h: No such file or directory` | Файл скомпилирован напрямую (кнопка Run C++ File / g++ main.cpp). Собирать нужно **только через CMake** (шаг 3) — пути к zlib задаёт CMakeLists.txt. Ставить zlib в систему не нужно, он лежит в `third_party/`. |
| `nmake : no such file or directory` | Конфигурация без компилятора. Используйте генератор `-G "Visual Studio 17 2022" -A x64` (шаг 3). |
| F5 ругается на `cppvsdbg` | Не установлено расширение C/C++ (шаг 2) или собрано без Release-конфигурации (шаг 3). |

## Структура репозитория

```text
├── prompt/            # промпт, ответ Qwen-Coder, имя модели
├── vulnerable/        # дефектная версия (zlib 1.2.11, CWE-120 + CWE-78)
│   ├── CMakeLists.txt
│   ├── src/main.cpp
│   └── third_party/zlib-1.2.11/
├── fixed/             # исправленная версия (zlib 1.3.1)
│   ├── CMakeLists.txt
│   ├── src/main.cpp
│   └── third_party/zlib-1.3.1/
├── samples/sample.txt # тестовый вход
├── reports/           # versions.txt, commands.md, findings.md, comparison.md,
│   ├── vulnerable/    #   owasp-bdu.md + логи cppcheck/flawfinder/clang-SA до и после
│   └── fixed/
└── .vscode/           # extensions.json, tasks.json, launch.json, c_cpp_properties.json
```

## О намеренных уязвимостях (vulnerable/)

| # | Строка | CWE | Описание | Исправление в fixed/ |
|---|--------|-----|----------|----------------------|
| 1 | main.cpp:54 | CWE-120 | `strcpy(comment, first_line)` в `char comment[64]` без проверки длины | усечение `std::string::substr(0, 63)` |
| 2 | main.cpp:91 | CWE-78 | `std::system("type \"" + input_path + "\"")` без санитизации | вызов оболочки удалён, проверка через `std::filesystem` |

Зависимость zlib 1.2.11 уязвима к **CVE-2018-25032** (heap overflow в deflate,
затронуты версии < 1.2.12; проверено вручную по
[NVD](https://nvd.nist.gov/vuln/detail/CVE-2018-25032)). В `fixed/` обновлена до 1.3.1.

Примечание: отчёты SAST сгенерированы в Linux-среде, поэтому третьим анализатором
вместо MSVC `/analyze` выступает Clang Static Analyzer; команды MSVC для Windows
приведены в `reports/commands.md` и в задачах VS Code.
