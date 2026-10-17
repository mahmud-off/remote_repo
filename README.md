# Лабораторная работа РБПО: SAST-анализ C++-приложения

Учебное приложение **«textzip»** — простое сжатие текста через zlib,
сгенерировано LLM (**Qwen-Coder**, см. `prompt/`), содержит две намеренные
уязвимости разных типов для последующего поиска статическими анализаторами,
исправления и сравнения трёх SAST-инструментов.

## Структура

```text
rbpo-lab/
├── README.md
├── prompt/
│   ├── prompt.md              # промпт, отправленный в Qwen-Coder
│   ├── qwen-output.md         # сохранённый ответ модели (без правок кода)
│   └── model-name.txt         # название модели (версия в интерфейсе не отображается)
├── vulnerable/                # дефектная версия (сгенерирована LLM)
│   ├── CMakeLists.txt
│   ├── src/main.cpp           # strcpy (CWE-120) + system (CWE-78)
│   └── third_party/zlib-1.2.11/   # зависимость с CVE-2018-25032
├── fixed/                     # исправленная версия
│   ├── CMakeLists.txt
│   ├── src/main.cpp           # безопасное усечение, system удалён
│   └── third_party/zlib-1.3.1/    # обновлённая зависимость
├── reports/
│   ├── versions.txt           # версии инструментов и зависимостей
│   ├── commands.md            # все команды сборки и запуска SAST
│   ├── findings.md            # разбор находок (TP/FP, CWE, пропуски)
│   ├── comparison.md          # сравнение трёх SAST до/после
│   ├── owasp-bdu.md           # OWASP Top 10 и поиск по БДУ ФСТЭК
│   ├── vulnerable/            # логи трёх SAST на дефектной версии
│   │   ├── cppcheck.txt
│   │   ├── clang-sa-console.txt
│   │   └── flawfinder.txt
│   └── fixed/                 # логи трёх SAST на исправленной версии
│       ├── cppcheck.txt
│       ├── clang-sa-console.txt
│       └── flawfinder.txt
├── .vscode/                   # настройки VS Code: запуск (F5) и задачи SAST
│   ├── launch.json            # конфигурации запуска vulnerable/fixed
│   ├── tasks.json             # задачи: сборка CMake + три SAST
│   └── extensions.json        # рекомендуемые расширения
└── samples/sample.txt
```

## Приложение

Функциональность: читает текст из файла или stdin (`-`), сжимает его zlib
(`compress2`), пишет результат в выходной файл, печатает размер до/после.

Три SAST-анализатора: **Cppcheck 2.10**, **Clang Static Analyzer 14**
(замена MSVC `cl /analyze`, недоступного в Linux-окружении проверки; детали —
в `reports/commands.md`), **Flawfinder 2.0.20**.

Намеренные ошибки в `vulnerable/`:

1. `strcpy(comment, first_line.c_str())` в `char comment[64]` — CWE-120;
2. `std::system(("type " + input_path).c_str())` — CWE-78.

Зависимость: zlib **1.2.11** с известной **CVE-2018-25032** (проверено по NVD,
см. `reports/owasp-bdu.md`). В `fixed/` обновлена до **1.3.1**.

## Запуск из VS Code (Windows, подробная инструкция)

### Шаг 1. Установить необходимое ПО

1. **Visual Studio Build Tools 2022** (самый важный пункт — в нём компилятор C++,
   которого нет в «голом» VS Code):
   - скачать: https://visualstudio.microsoft.com/ru/downloads/ → «Visual Studio
     2022 Tools for Windows (x64)» (или через `winget install Microsoft.VisualStudio.2022.BuildTools`);
   - в установщике выбрать workload **«Desktop development with C++»**;
   - в правом окне «Компоненты» дополнительно проверить:
     - *MSVC v143 – VS 2022 C++ x64/x86 build tools*
     - *Windows 10/11 SDK*
     - *C++ CMake tools for Windows* (даст CMake внутри VS-окружения)
     - *C++ AddressSanitizer* (по желанию).
2. **CMake ≥ 3.20**, если не поставили его вместе с Build Tools:
   `winget install Kitware.CMake`.
3. **SAST-инструменты**:
   ```powershell
   winget install Cppcheck.Cppcheck
   winget install Python.Python.3.12
   python -m pip install --upgrade flawfinder
   ```
4. **Расширения VS Code** (в репозитории есть `.vscode/extensions.json`, VS Code
   сам предложит установить):
   - **C/C++** (`ms-vscode.cpptools`) — запуск и отладка;
   - **CMake Tools** (`ms-vscode.cmake-tools`).

Проверка после установки (обычный PowerShell может не видеть `cl` — это нормально,
для `cl` используйте Developer PowerShell, см. шаг 4):

```powershell
cmake --version
cppcheck --version
python -m flawfinder --version
```

### Шаг 2. Открыть проект и настроить терминал

1. Откройте папку репозитория в VS Code (**File → Open Folder**).
2. Терминал: **Terminal → New Terminal**. Если обычный PowerShell не находит
   `cmake`, переключитесь: **Ctrl+Shift+P → “Terminal: Select Default Profile” →
   «Developer PowerShell for VS 2022»** (там видны и `cmake`, и `cl`).

### Шаг 3. Конфигурация и сборка (терминал или задачи VS Code)

Вариант А — команды в терминале:

⚠️ Если папка `build\vulnerable` (или `build\fixed`) уже создавалась ранее с
ДРУГИМ генератором (например, по умолчанию получился «NMake Makefiles»), CMake
сообщит `Does not match the generator used previously`. В этом случае сначала
удалите старый кэш сборки и конфигурируйте заново:

```powershell
Remove-Item -Recurse -Force build   # или просто build\vulnerable и build\fixed
cmake -S vulnerable -B build/vulnerable -G "Visual Studio 17 2022" -A x64
cmake --build build/vulnerable --config Release

cmake -S fixed -B build/fixed -G "Visual Studio 17 2022" -A x64
cmake --build build/fixed --config Release
```

Вариант Б — задачи VS Code: **Terminal → Run Task…**:
- «CMake: configure vulnerable», «CMake: build vulnerable (Release)»;
- «CMake: configure fixed», «CMake: build fixed (Release)».

После сборки появятся `build\vulnerable\Release\textzip.exe` и
`build\fixed\Release\textzip.exe`.

### Шаг 4. Запуск приложения

Вариант А — из терминала (обратный слэш, расширение `.exe`, вместо `printf` —
`echo`):

```powershell
.\build\vulnerable\Release\textzip.exe samples\sample.txt out_vulnerable.z
"hello stdin" | .\build\vulnerable\Release\textzip.exe - out_stdin.z

.\build\fixed\Release\textzip.exe samples\sample.txt out_fixed.z
"hello fixed stdin" | .\build\fixed\Release\textzip.exe - out_fixed_stdin.z
```

Ожидаемый вывод: первая строка входа как «Comment», размеры до/после сжатия,
содержимое входного файла (это сработавшая инъекция команд — `type` в Windows /
`cat` в Linux), созданный `.z`-файл.

Вариант Б — кнопки запуска VS Code: **Run → Start Debugging (F5)**, в выпадающем
списке слева выбрать одну из конфигураций (настроены в `.vscode/launch.json`):
- «textzip: vulnerable (file input)»
- «textzip: vulnerable (stdin)»
- «textzip: fixed (file input)»
- «textzip: fixed (stdin)»

Каждая конфигурация сначала автоматически собирает проект (задача
`preLaunchTask`), затем запускает `.exe` с готовыми аргументами во встроенном
терминале. Для конфигураций со stdin наберите текст в терминале и нажмите
**Ctrl+Z + Enter** — так в Windows завершается ввод с клавиатуры.

### Шаг 5. Запуск трёх SAST из VS Code

Задачи уже настроены — **Terminal → Run Task… → «SAST (vulnerable): run all»** и
«SAST (fixed): run all» (или по отдельности cppcheck / cl /analyze / flawfinder).
Находки Cppcheck подсвечиваются на строках кода (problem matcher).

⚠️ Задача «MSVC cl /analyze» работает только если VS Code запущен из
**Developer PowerShell for VS 2022** (ярлык в меню Пуск:
`cd <папка_репозитория>` затем `code .`) — иначе команда `cl` не найдётся.
Тогда же сохраните логи с перенаправлением вывода, как в `reports/commands.md`.

### Возможные проблемы

| Симптом | Причина и решение |
|---|---|
| `cmake : Имя "cmake" не распознано...` | CMake не установлен или не в PATH. Установите `winget install Kitware.CMake` и перезапустите VS Code, либо работайте в Developer PowerShell. |
| `Running 'nmake' '-?' failed with: no such file or directory` | Нет компилятора C++. Установите Build Tools 2022 (workload «Desktop development with C++») и указывайте генератор явно: `-G "Visual Studio 17 2022" -A x64`. |
| `Error: generator : Visual Studio 17 2022 ... Does not match the generator used previously: NMake Makefiles` | Папка `build\...` уже была сконфигурирована другим генератором. Удалите кэш: `Remove-Item -Recurse -Force build`, затем конфигурируйте заново с нужным `-G`. Генератор нельзя менять в существующей папке сборки — только с чистой. |
| `cl` не находится в терминале | Обычный PowerShell не содержит среду MSVC. Используйте Developer PowerShell for VS 2022. |
| `./build/.../textzip: не распознано` | В PowerShell уберите `./`, добавьте `.exe` и обратные слэши: `.\build\vulnerable\Release\textzip.exe`. |
| `printf` не найден | Это Unix-команда. В PowerShell: `"текст" \| ...textzip.exe - out.z`. |
| Линтер в редакторе подчёркивает `<zlib.h>` красным | IntelliSense не знает include-путей. Добавьте в `.vscode/settings.json`: `"C_Cpp.default.compileCommands": "${workspaceFolder}/build/vulnerable/compile_commands.json"` (для этого конфигурируйте с `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`) — на сборку и запуск это не влияет. |

## Сборка и запуск

### Linux (на ней фактически выполнялась проверка)

```bash
cmake -S vulnerable -B build/vulnerable && cmake --build build/vulnerable -j4
./build/vulnerable/textzip samples/sample.txt out_vulnerable.z
printf 'hello stdin\n' | ./build/vulnerable/textzip - out_stdin.z

cmake -S fixed -B build/fixed && cmake --build build/fixed -j4
./build/fixed/textzip samples/sample.txt out_fixed.z
```

Проверено: обе версии собираются и корректно работают на коротком файле и stdin
(вывод размеров до/после сжатия присутствует).

## Запуск SAST

Полные команды — в `reports/commands.md`. Кратко:

```bash
cppcheck --enable=all --std=c++17 --force -I vulnerable/src \
  -I vulnerable/third_party/zlib-1.2.11 vulnerable/src
clang --analyze -Xclang -analyzer-checker=security,core,unix -std=c++17 -x c++ \
  -I vulnerable/third_party/zlib-1.2.11 -c vulnerable/src/main.cpp -o /tmp/main.o
flawfinder --minlevel=1 vulnerable/src
```

## Основные результаты

- До исправления: Flawfinder нашёл обе намеренные ошибки, Clang SA — ошибку
  памяти, Cppcheck — не нашёл ни одной (только style/config шум).
- После исправления: все TP-находки исчезли (Flawfinder — «No hits found»,
  Clang SA — 0 предупреждений, Cppcheck — только прежний стилистический шум).
- Подробный разбор: `reports/findings.md`, сравнение: `reports/comparison.md`,
  привязка к OWASP/БДУ: `reports/owasp-bdu.md`.
