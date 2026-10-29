#pragma once

#include <string>

namespace hisab::security {

class PasswordHasher {
public:
    static std::string hashPassword(const std::string& password);

    static bool verifyPassword(
        const std::string& password,
        const std::string& storedHash
    );

    static constexpr int kIterations = 600000;
    static constexpr int kSaltBytes = 16;
    static constexpr int kDerivedKeyBytes = 32;
};

} // namespace hisab::security