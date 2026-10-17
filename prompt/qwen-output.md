# Ответ Qwen-Coder (сохранённый результат генерации)

Model: Qwen-Coder (точная версия не отображается в интерфейсе; см. `model-name.txt`)

Ниже — ответ модели на промпт из `prompt.md`. Код сохранён без изменений и
используется как основа для версии `vulnerable/`.

---

**Модель:** Qwen-Coder (версия в интерфейсе не отображается)

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(textzip C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ZLIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib-1.2.11)

add_library(zlib_static STATIC
    ${ZLIB_SRC_DIR}/adler32.c
    ${ZLIB_SRC_DIR}/compress.c
    ${ZLIB_SRC_DIR}/crc32.c
    ${ZLIB_SRC_DIR}/deflate.c
    ${ZLIB_SRC_DIR}/gzclose.c
    ${ZLIB_SRC_DIR}/gzlib.c
    ${ZLIB_SRC_DIR}/gzread.c
    ${ZLIB_SRC_DIR}/gzwrite.c
    ${ZLIB_SRC_DIR}/infback.c
    ${ZLIB_SRC_DIR}/inffast.c
    ${ZLIB_SRC_DIR}/inflate.c
    ${ZLIB_SRC_DIR}/inftrees.c
    ${ZLIB_SRC_DIR}/trees.c
    ${ZLIB_SRC_DIR}/uncompr.c
    ${ZLIB_SRC_DIR}/zutil.c
)

target_include_directories(zlib_static PUBLIC ${ZLIB_SRC_DIR})

if(MSVC)
    target_compile_definitions(zlib_static PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_executable(textzip src/main.cpp)
target_link_libraries(textzip PRIVATE zlib_static)
```

## src/main.cpp

```cpp
// textzip — учебное приложение: сжатие текста через zlib (Qwen-Coder, lab RBPO)
//
// Зависимость: zlib 1.2.11 (third_party/zlib-1.2.11).
// Для zlib < 1.2.12 известна CVE-2018-25032 (heap buffer overflow / use after
// free в deflate), см. https://nvd.nist.gov/vuln/detail/CVE-2018-25032
//
// Использование:
//   textzip <input_file|-> <output_file>
//   "-" означает чтение из stdin.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <zlib.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <input_file|-> <output_file>" << std::endl;
        return 1;
    }

    const std::string input_path = argv[1];
    const std::string output_path = argv[2];

    // 1. Чтение входных данных (файл или stdin)
    std::string data;
    if (input_path == "-") {
        std::string line;
        while (std::getline(std::cin, line)) {
            data += line;
            data += "\n";
        }
    } else {
        std::ifstream in(input_path, std::ios::binary);
        if (!in) {
            std::cerr << "Cannot open input file: " << input_path << std::endl;
            return 1;
        }
        data.assign(std::istreambuf_iterator<char>(in),
                    std::istreambuf_iterator<char>());
    }

    // 2. INTENTIONAL SECURITY BUG FOR LAB (memory error, CWE-120):
    //    копируем первую строку входа в фиксированный буфер без проверки длины.
    char comment[64];
    std::string first_line;
    std::size_t nl = data.find('\n');
    first_line = (nl == std::string::npos) ? data : data.substr(0, nl);
    strcpy(comment, first_line.c_str()); // no bounds checking on purpose
    std::cout << "Comment (first line): " << comment << std::endl;

    // 3. Сжатие через zlib (реальное использование зависимости)
    uLongf bound = compressBound(static_cast<uLong>(data.size()));
    std::string compressed(bound, '\0');
    int rc = compress2(reinterpret_cast<Bytef*>(&compressed[0]), &bound,
                       reinterpret_cast<const Bytef*>(data.data()),
                       static_cast<uLong>(data.size()), Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK) {
        std::cerr << "zlib compress2 failed: " << rc << std::endl;
        return 1;
    }
    compressed.resize(bound);

    // 4. Запись результата
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot open output file: " << output_path << std::endl;
        return 1;
    }
    out.write(compressed.data(), static_cast<std::streamsize>(compressed.size()));
    out.close();

    std::cout << "Original size:  " << data.size() << " bytes" << std::endl;
    std::cout << "Compressed size:" << compressed.size() << " bytes" << std::endl;

    // 5. INTENTIONAL SECURITY BUG FOR LAB (OS command injection, CWE-78):
    //    склеиваем имя входного файла с командой ОС без экранирования.
    if (input_path != "-") {
        std::system(("type " + input_path).c_str()); // no sanitization on purpose
    }

    return 0;
}
```

## Инструкция по сборке на Windows

```powershell
cmake -S vulnerable -B build\vulnerable -G "Visual Studio 17 2022" -A x64
cmake --build build\vulnerable --config Release
.\build\vulnerable\Release\textzip.exe samples\sample.txt out.z
```

Требуется Visual Studio Build Tools 2022 (компонент “Desktop development with C++”)
и CMake ≥ 3.20. Исходники zlib 1.2.11 должны лежать в `third_party/zlib-1.2.11`.

## Список намеренных ошибок

1. Переполнение буфера: `strcpy(comment, first_line.c_str())` в фиксированный
   `char comment[64]` без проверки длины — **CWE-120** (Buffer Copy without
   Checking Size of Input), потенциально CWE-787 (Out-of-bounds Write).
2. Инъекция команд ОС: `std::system(("type " + input_path).c_str())` без
   экранирования имени файла — **CWE-78** (OS Command Injection).

## Версия зависимости и CVE

- zlib **1.2.11**;
- предлагаемый к проверке CVE: **CVE-2018-25032** (deflate heap buffer
  overflow / use-after-free, затронуты версии до 1.2.12).
