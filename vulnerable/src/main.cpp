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
