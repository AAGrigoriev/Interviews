#include <array>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <optional>

#include <openssl/sha.h>


namespace test_work {

constexpr std::size_t magic_size = 1024; // Магический размер буффера.

using sha_buffer = std::array<unsigned char, SHA256_DIGEST_LENGTH>;
using read_buffer = std::array<char, magic_size>;

std::optional<sha_buffer> sha256_from_input() {
    // Cначала настроим всё что нужно для чтения.
    if (!std::freopen(nullptr, "rb", stdin)) {
        return {};
    }

    SHA256_CTX context;
    if (!SHA256_Init(&context)) {
        return {};
    }

    // Попробуем сишным путём.
    std::size_t bytes_read;
    read_buffer buffer;
    while((bytes_read = std::fread(buffer.data(), sizeof(buffer.front()), buffer.size(), stdin)) > 0) {
        if(std::ferror(stdin) && !std::feof(stdin)) {
            return {};
        }

        if (!SHA256_Update(&context, buffer.data(), bytes_read)) {
            return {};
        }
    }

    sha_buffer result;
    if (!SHA256_Final(result.data(), &context)) {
        return {};
    }

    return result;
}

} // namespace test_work


int main() {
    // todo: Подумать над ошибками, возможно нужно какие то конткретные сообщения выводить. В задаче не указано).
    if (const auto result = test_work::sha256_from_input()) {
        for (const auto byte : *result) {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(byte);
        }
        std::cout << std::endl;
    } else {
        std::cout << "oops something wrong!!" << std::endl;
    }

    return 0;
}
