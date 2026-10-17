# Команды запуска сборки и SAST-анализа

Все команды выполнялись из корня репозитория. Отчёты сохранялись через перенаправление вывода (`tee`).

## Замечание про окружение

План лабораторной рассчитан на Windows + MSVC (`cl /analyze`). Фактическая проверка
выполнялась в Linux-окружении (Debian, gcc 12), поэтому вместо Microsoft Visual
Studio Code Analysis использован **Clang Static Analyzer** (`clang --analyze`) —
это тоже независимый межпроцедурный SAST с анализом путей выполнения. Три разных
SAST-инструмента при этом сохранены: Cppcheck, Clang Static Analyzer, Flawfinder.
Команды для Windows/MSVC приведены в конце файла для полноты.

## Сборка и запуск

```powershell
# уязвимая версия
cmake -S vulnerable -B build/vulnerable
cmake --build build/vulnerable -j4
./build/vulnerable/textzip samples/sample.txt out_vulnerable.z
printf 'hello stdin\n' | ./build/vulnerable/textzip - out_stdin.z

# исправленная версия
cmake -S fixed -B build/fixed
cmake --build build/fixed -j4
./build/fixed/textzip samples/sample.txt out_fixed.z
printf 'hello fixed stdin\n' | ./build/fixed/textzip - out_fixed_stdin.z
```

## Версии инструментов

Файл `reports/versions.txt` собран командой:

```bash
{ cppcheck --version; flawfinder --version | head -1; clang --version | head -1;
  gcc --version | head -1; cmake --version | head -1; python3 --version; } > reports/versions.txt
```

## 1. Cppcheck

Уязвимая версия (лог: `reports/vulnerable/cppcheck.txt`):

```bash
cppcheck --enable=all --std=c++17 --language=c++ --inline-suppr --template=gcc \
  --suppress=missingIncludeSystem --force \
  -I vulnerable/src -I vulnerable/third_party/zlib-1.2.11 vulnerable/src \
  2>&1 | tee reports/vulnerable/cppcheck.txt
```

Исправленная версия (лог: `reports/fixed/cppcheck.txt`):

```bash
cppcheck --enable=all --std=c++17 --language=c++ --inline-suppr --template=gcc \
  --suppress=missingIncludeSystem --force \
  -I fixed/src -I fixed/third_party/zlib-1.3.1 fixed/src \
  2>&1 | tee reports/fixed/cppcheck.txt
```

Примечание: флаг `--force` добавлен, потому что без него Cppcheck проверяет
только одну конфигурацию препроцессора из-за обилия `#ifdef` в `zconf.h`
(предупреждение `toomanyconfigs`).

## 2. Clang Static Analyzer (вместо MSVC `cl /analyze`)

Уязвимая версия (лог: `reports/vulnerable/clang-sa-console.txt`):

```bash
clang --analyze -Xclang -analyzer-checker=security,core,unix \
  -std=c++17 -x c++ -I vulnerable/third_party/zlib-1.2.11 \
  -c vulnerable/src/main.cpp -o /tmp/scan-vuln/main.o \
  2>&1 | tee reports/vulnerable/clang-sa-console.txt
```

Исправленная версия (лог: `reports/fixed/clang-sa-console.txt`):

```bash
clang --analyze -Xclang -analyzer-checker=security,core,unix \
  -std=c++17 -x c++ -I fixed/third_party/zlib-1.3.1 \
  -c fixed/src/main.cpp -o /tmp/scan-vuln/main_fixed.o \
  2>&1 | tee reports/fixed/clang-sa-console.txt
```

## 3. Flawfinder

Уязвимая версия (лог: `reports/vulnerable/flawfinder.txt`):

```bash
flawfinder --minlevel=1 vulnerable/src 2>&1 | tee reports/vulnerable/flawfinder.txt
```

Исправленная версия (лог: `reports/fixed/flawfinder.txt`):

```bash
flawfinder --minlevel=1 fixed/src 2>&1 | tee reports/fixed/flawfinder.txt
```

## Проверка версий зависимости

```bash
grep -m1 '#define ZLIB_VERSION' vulnerable/third_party/zlib-1.2.11/zlib.h   # "1.2.11"
grep -m1 '#define ZLIB_VERSION' fixed/third_party/zlib-1.3.1/zlib.h         # "1.3.1"
grep -nE "zlib.h|compress|gz" vulnerable/src/main.cpp                        # zlib реально используется
```

## Эквивалент для Windows / MSVC (из исходного плана)

```powershell
cl /nologo /std:c++17 /EHsc /c /I vulnerable\src /I vulnerable\third_party\zlib-1.2.11 `
   /D_CRT_SECURE_NO_WARNINGS /analyze /analyze:log reports\vulnerable\msvc-sast.xml `
   vulnerable\src\main.cpp 2>&1 | Tee-Object -FilePath reports\vulnerable\msvc-sast-console.txt
```

(В данном окружении MSVC недоступен, поэтому этот шаг заменён Clang Static Analyzer.)
