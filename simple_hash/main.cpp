#include <array>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

#include <openssl/sha.h>


namespace test_work {

using sha_buffer = std::array<unsigned char, SHA256_DIGEST_LENGTH>;

std::optional<sha_buffer> sha256_from_input() {
    SHA256_CTX context;
    if (!SHA256_Init(&context)) {
        return {};
    }

    for (std::string line; std::getline(std::cin, line); ) {
        if (!SHA256_Update(&context, line.data(), line.size())) {
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
        std::cout << std::hex;
        for (const auto byte : *result) {
            std::cout << static_cast<int>(byte);
        }
        std::cout << std::endl;
    } else {
        std::cout << "wrong data" << std::endl;
    }

    return 0;
}
