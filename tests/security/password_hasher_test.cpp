#include "security/PasswordHasher.h"

#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    try {
        const std::string password = "HisabTestPassword123!";

        std::cout << "Testing password hashing...\n";

        const std::string hash =
            hisab::security::PasswordHasher::hashPassword(password);

        if (hash.empty()) {
            throw std::runtime_error(
                "Password hash is empty."
            );
        }

        std::cout << "Password hash generated successfully.\n";
        std::cout << "Hash format: " << hash << '\n';

        if (!hisab::security::PasswordHasher::verifyPassword(
                password,
                hash
            )) {
            throw std::runtime_error(
                "Correct password was rejected."
            );
        }

        std::cout << "Correct password verified successfully.\n";

        if (hisab::security::PasswordHasher::verifyPassword(
                "WrongPassword123!",
                hash
            )) {
            throw std::runtime_error(
                "Incorrect password was accepted."
            );
        }

        std::cout << "Incorrect password rejected successfully.\n";

        const std::string secondHash =
            hisab::security::PasswordHasher::hashPassword(password);

        if (hash == secondHash) {
            throw std::runtime_error(
                "Two password hashes are identical. "
                "Salt generation may not be working correctly."
            );
        }

        std::cout << "Unique salt/hash generation verified.\n";

        if (hisab::security::PasswordHasher::verifyPassword(
                password,
                secondHash
            ) == false) {
            throw std::runtime_error(
                "Second generated hash failed verification."
            );
        }

        std::cout << "Second hash verified successfully.\n";

        if (hisab::security::PasswordHasher::verifyPassword(
                password,
                ""
            )) {
            throw std::runtime_error(
                "Empty stored hash was accepted."
            );
        }

        std::cout << "Invalid/empty hash rejected successfully.\n";

        std::cout << "\nPassword hasher test PASSED.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "\nPassword hasher test FAILED:\n";
        std::cerr << error.what() << '\n';

        return 1;
    }
}