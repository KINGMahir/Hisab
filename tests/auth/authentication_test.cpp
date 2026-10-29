#include "auth/AuthenticationService.h"
#include "database/Database.h"
#include "database/Migrations.h"
#include "security/PasswordHasher.h"

#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    try {
        const std::string databasePath =
            "hisab_auth_test.db";

        hisab::database::Database database(databasePath);

        std::cout << "Opening authentication test database...\n";
        database.open();

        if (!database.isOpen()) {
            throw std::runtime_error(
                "Database did not open."
            );
        }

        std::cout << "Database opened successfully.\n";

        std::cout << "Running migrations...\n";
        hisab::database::Migrations::run(database);

        std::cout << "Migrations completed successfully.\n";

        const std::string password =
            "TestPassword123!";

        const std::string passwordHash =
            hisab::security::PasswordHasher::hashPassword(
                password
            );

        std::cout << "Creating test user...\n";

        database.beginTransaction();

        database.execute(
            "INSERT INTO users "
            "(employee_id, username, password_hash, display_name) "
            "VALUES "
            "('EMP001', "
            "'testuser', "
            "'" + passwordHash + "', "
            "'Test Employee');"
        );

        database.commit();

        std::cout << "Test user created successfully.\n";

        hisab::auth::AuthenticationService authentication(
            database
        );

        std::cout << "\nTesting successful login...\n";

        const hisab::auth::AuthenticatedUser user =
            authentication.login(
                "testuser",
                password
            );

        if (user.id <= 0) {
            throw std::runtime_error(
                "Authenticated user has invalid ID."
            );
        }

        if (user.employeeId != "EMP001") {
            throw std::runtime_error(
                "Incorrect employee ID returned."
            );
        }

        if (user.username != "testuser") {
            throw std::runtime_error(
                "Incorrect username returned."
            );
        }

        if (user.displayName != "Test Employee") {
            throw std::runtime_error(
                "Incorrect display name returned."
            );
        }

        std::cout << "Successful login verified.\n";

        std::cout << "\nTesting incorrect password...\n";

        bool incorrectPasswordRejected = false;

        try {
            authentication.login(
                "testuser",
                "WrongPassword123!"
            );
        }
        catch (const std::runtime_error&) {
            incorrectPasswordRejected = true;
        }

        if (!incorrectPasswordRejected) {
            throw std::runtime_error(
                "Incorrect password was accepted."
            );
        }

        std::cout << "Incorrect password rejected successfully.\n";

        std::cout << "\nTesting unknown username...\n";

        bool unknownUserRejected = false;

        try {
            authentication.login(
                "unknownuser",
                password
            );
        }
        catch (const std::runtime_error&) {
            unknownUserRejected = true;
        }

        if (!unknownUserRejected) {
            throw std::runtime_error(
                "Unknown username was accepted."
            );
        }

        std::cout << "Unknown username rejected successfully.\n";

        std::cout << "\nTesting empty username...\n";

        bool emptyUsernameRejected = false;

        try {
            authentication.login(
                "",
                password
            );
        }
        catch (const std::invalid_argument&) {
            emptyUsernameRejected = true;
        }

        if (!emptyUsernameRejected) {
            throw std::runtime_error(
                "Empty username was not rejected."
            );
        }

        std::cout << "Empty username rejected successfully.\n";

        std::cout << "\nTesting empty password...\n";

        bool emptyPasswordRejected = false;

        try {
            authentication.login(
                "testuser",
                ""
            );
        }
        catch (const std::invalid_argument&) {
            emptyPasswordRejected = true;
        }

        if (!emptyPasswordRejected) {
            throw std::runtime_error(
                "Empty password was not rejected."
            );
        }

        std::cout << "Empty password rejected successfully.\n";

        database.close();

        std::cout << "\nAuthentication test PASSED.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "\nAuthentication test FAILED:\n";
        std::cerr << error.what() << '\n';

        return 1;
    }
}