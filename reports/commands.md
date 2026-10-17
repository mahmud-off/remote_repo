# Команды запуска SAST-анализаторов

Отчёты в этой папке сгенерированы в Linux-среде (сборочный контейнер), поэтому
вместо Microsoft Visual Studio Code Analysis (MSVC `cl /analyze`) использован
**Clang Static Analyzer** — третий независимый SAST, доступный кроссплатформенно.
Ниже приведены команды для обеих платформ.

## Linux / WSL / macOS (использовались для генерации отчётов)

### Cppcheck
```bash
cppcheck --force --enable=all --std=c++17 --language=c++ --inline-suppr \
  --template=gcc --suppress=missingIncludeSystem --suppress=ConfigurationNotChecked \
  -I vulnerable/third_party/zlib-1.2.11 vulnerable/src \
  > reports/vulnerable/cppcheck.txt 2>&1

cppcheck --force --enable=all --std=c++17 --language=c++ --inline-suppr \
  --template=gcc --suppress=missingIncludeSystem --suppress=ConfigurationNotChecked \
  -I fixed/third_party/zlib-1.3.1 fixed/src \
  > reports/fixed/cppcheck.txt 2>&1
```

### Clang Static Analyzer
```bash
clang --analyze -std=c++17 -I vulnerable/third_party/zlib-1.2.11 \
  vulnerable/src/main.cpp > reports/vulnerable/clang-sa-console.txt 2>&1

clang --analyze -std=c++17 -I fixed/third_party/zlib-1.3.1 \
  fixed/src/main.cpp > reports/fixed/clang-sa-console.txt 2>&1
```

### Flawfinder
```bash
python3 -m flawfinder --minlevel=1 vulnerable/src > reports/vulnerable/flawfinder.txt 2>&1
python3 -m flawfinder --minlevel=1 fixed/src      > reports/fixed/flawfinder.txt 2>&1
```

## Windows (PowerShell) — эквиваленты, включая MSVC

### Cppcheck
```powershell
cppcheck --force --enable=all --std=c++17 --language=c++ --inline-suppr --template=gcc --suppress=missingIncludeSystem --suppress=ConfigurationNotChecked -I vulnerable\third_party\zlib-1.2.11 vulnerable\src 2>&1 | Tee-Object -FilePath reports\vulnerable\cppcheck.txt
```

### Microsoft Visual Studio Code Analysis (`cl /analyze`, Developer PowerShell for VS 2022)
```powershell
cl /nologo /std:c++17 /EHsc /c /I vulnerable\third_party\zlib-1.2.11 /D_CRT_SECURE_NO_WARNINGS /analyze /analyze:log reports\vulnerable\msvc-sast.xml vulnerable\src\main.cpp 2>&1 | Tee-Object -FilePath reports\vulnerable\msvc-sast-console.txt
```

### Clang Static Analyzer (если установлен clang для Windows)
```powershell
clang --analyze -std=c++17 -I vulnerable\third_party\zlib-1.2.11 vulnerable\src\main.cpp 2>&1 | Tee-Object -FilePath reports\vulnerable\clang-sa-console.txt
```

### Flawfinder
```powershell
python -m flawfinder --minlevel=1 vulnerable\src 2>&1 | Tee-Object -FilePath reports\vulnerable\flawfinder.txt
```

Аналогичные команды с заменой `vulnerable` -> `fixed` и `zlib-1.2.11` -> `zlib-1.3.1`
использованы для повторного анализа исправленной версии.
