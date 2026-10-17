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

## Сборка (Windows / MSVC)

Требуется Visual Studio Build Tools 2022 («Desktop development with C++») и CMake ≥ 3.20.

```powershell
cmake -S vulnerable -B build\vulnerable -G "Visual Studio 17 2022" -A x64
cmake --build build\vulnerable --config Release
.\build\vulnerable\Release\textzip.exe samples\sample.txt out_vulnerable.z
"hello stdin" | .\build\vulnerable\Release\textzip.exe - out_stdin.z

cmake -S fixed -B build\fixed -G "Visual Studio 17 2022" -A x64
cmake --build build\fixed --config Release
.\build\fixed\Release\textzip.exe samples\sample.txt out_fixed.z
```

## Сборка (Linux — на ней фактически выполнялась проверка)

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
