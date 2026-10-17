// textzip — учебное приложение: сжатие текста через zlib (исправленная версия)
//
// Исправления по сравнению с vulnerable/:
//   1. CWE-120 (переполнение буфера через strcpy в char comment[64]):
//      заменено на std::string с контролируемым усечением до 63 символов.
//   2. CWE-78 (инъекция команд через std::system("type " + input_path)):
//      std::system удалён полностью; проверка существования файла выполняется
//      через std::filesystem без вызова командной оболочки.
//   3. Зависимость обновлена: zlib 1.2.11 -> zlib 1.3.1
//      (CVE-2018-25032 исправлен в zlib 1.2.12, см.
//       https://nvd.nist.gov/vuln/detail/CVE-2018-25032).
//
// Использование:
//   textzip <input_file|-> <output_file>
//   "-" означает чтение из stdin.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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

    // 2. FIXED (было CWE-120, strcpy в char comment[64]):
    //    размер контролируется, неконтролируемого копирования нет.
    std::size_t nl = data.find('\n');
    std::string first_line = (nl == std::string::npos) ? data : data.substr(0, nl);
    std::string comment = first_line.substr(0, 63); // безопасное усечение
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

    // 5. FIXED (было CWE-78, std::system("type " + input_path)):
    //    вместо вызова командной оболочки — безопасная проверка через std::filesystem.
    if (input_path != "-") {
        std::error_code ec;
        if (std::filesystem::exists(input_path, ec)) {
            std::cout << "Input file exists: " << input_path << std::endl;
        } else {
            std::cout << "Input file not found: " << input_path << std::endl;
        }
    }

    return 0;
}
