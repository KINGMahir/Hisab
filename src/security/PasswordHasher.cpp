#include "PasswordHasher.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace hisab::security {

namespace {

std::string bytesToHex(
    const unsigned char* data,
    std::size_t length
) {
    std::ostringstream output;

    output << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < length; ++i) {
        output << std::setw(2)
               << static_cast<unsigned int>(data[i]);
    }

    return output.str();
}

std::vector<unsigned char> hexToBytes(
    const std::string& hex
) {
    if (hex.empty() || (hex.size() % 2) != 0) {
        throw std::runtime_error("Invalid hexadecimal data.");
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2);

    for (std::size_t i = 0; i < hex.size(); i += 2) {
        unsigned int value = 0;

        std::istringstream input(hex.substr(i, 2));
        input >> std::hex >> value;

        if (input.fail()) {
            throw std::runtime_error("Invalid hexadecimal data.");
        }

        bytes.push_back(
            static_cast<unsigned char>(value)
        );
    }

    return bytes;
}

std::string createHash(
    const std::string& password,
    int iterations
) {
    std::vector<unsigned char> salt(
        PasswordHasher::kSaltBytes
    );

    if (RAND_bytes(
            salt.data(),
            static_cast<int>(salt.size())
        ) != 1) {
        throw std::runtime_error(
            "Failed to generate cryptographically secure salt."
        );
    }

    std::vector<unsigned char> derivedKey(
        PasswordHasher::kDerivedKeyBytes
    );

    const int result = PKCS5_PBKDF2_HMAC(
        password.data(),
        static_cast<int>(password.size()),
        salt.data(),
        static_cast<int>(salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(derivedKey.size()),
        derivedKey.data()
    );

    if (result != 1) {
        throw std::runtime_error(
            "Password key derivation failed."
        );
    }

    return
        "pbkdf2_sha256$1$" +
        std::to_string(iterations) +
        "$" +
        bytesToHex(salt.data(), salt.size()) +
        "$" +
        bytesToHex(derivedKey.data(), derivedKey.size());
}

std::vector<std::string> split(
    const std::string& value,
    char delimiter
) {
    std::vector<std::string> parts;
    std::string current;

    for (const char character : value) {
        if (character == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current += character;
        }
    }

    parts.push_back(current);

    return parts;
}

} // namespace

std::string PasswordHasher::hashPassword(
    const std::string& password
) {
    if (password.empty()) {
        throw std::invalid_argument(
            "Password cannot be empty."
        );
    }

    return createHash(password, kIterations);
}

bool PasswordHasher::verifyPassword(
    const std::string& password,
    const std::string& storedHash
) {
    if (password.empty() || storedHash.empty()) {
        return false;
    }

    try {
        const std::vector<std::string> parts =
            split(storedHash, '$');

        if (parts.size() != 5) {
            return false;
        }

        if (parts[0] != "pbkdf2_sha256") {
            return false;
        }

        if (parts[1] != "1") {
            return false;
        }

        const int iterations =
            std::stoi(parts[2]);

        if (iterations <= 0) {
            return false;
        }

        const std::vector<unsigned char> salt =
            hexToBytes(parts[3]);

        const std::vector<unsigned char> expectedKey =
            hexToBytes(parts[4]);

        if (salt.size() != kSaltBytes) {
            return false;
        }

        if (expectedKey.size() != kDerivedKeyBytes) {
            return false;
        }

        std::vector<unsigned char> actualKey(
            kDerivedKeyBytes
        );

        const int result = PKCS5_PBKDF2_HMAC(
            password.data(),
            static_cast<int>(password.size()),
            salt.data(),
            static_cast<int>(salt.size()),
            iterations,
            EVP_sha256(),
            static_cast<int>(actualKey.size()),
            actualKey.data()
        );

        if (result != 1) {
            return false;
        }

        return CRYPTO_memcmp(
            actualKey.data(),
            expectedKey.data(),
            actualKey.size()
        ) == 0;
    }
    catch (...) {
        return false;
    }
}

} // namespace hisab::security