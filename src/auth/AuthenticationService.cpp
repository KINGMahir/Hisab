#include "AuthenticationService.h"

#include "security/PasswordHasher.h"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace hisab::auth {

AuthenticationService::AuthenticationService(
    database::Database& database
)
    : database_(database) {
}

AuthenticatedUser AuthenticationService::login(
    const std::string& username,
    const std::string& password
) {
    if (username.empty()) {
        throw std::invalid_argument(
            "Username cannot be empty."
        );
    }

    if (password.empty()) {
        throw std::invalid_argument(
            "Password cannot be empty."
        );
    }

    if (!database_.isOpen()) {
        throw std::runtime_error(
            "Cannot authenticate: database is not open."
        );
    }

    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "SELECT "
        "    id, "
        "    employee_id, "
        "    username, "
        "    password_hash, "
        "    display_name, "
        "    is_active "
        "FROM users "
        "WHERE username = ? "
        "LIMIT 1;";

    const int prepareResult = sqlite3_prepare_v2(
        database_.handle(),
        sql,
        -1,
        &statement,
        nullptr
    );

    if (prepareResult != SQLITE_OK) {
        throw std::runtime_error(
            "Failed to prepare authentication query: " +
            std::string(sqlite3_errmsg(database_.handle()))
        );
    }

    const int bindResult = sqlite3_bind_text(
        statement,
        1,
        username.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (bindResult != SQLITE_OK) {
        sqlite3_finalize(statement);

        throw std::runtime_error(
            "Failed to bind username for authentication."
        );
    }

    const int stepResult = sqlite3_step(statement);

    if (stepResult == SQLITE_DONE) {
        sqlite3_finalize(statement);

        throw std::runtime_error(
            "Invalid username or password."
        );
    }

    if (stepResult != SQLITE_ROW) {
        const std::string error =
            sqlite3_errmsg(database_.handle());

        sqlite3_finalize(statement);

        throw std::runtime_error(
            "Authentication query failed: " + error
        );
    }

    const int userId =
        sqlite3_column_int(statement, 0);

    const char* employeeIdText =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 1)
        );

    const char* usernameText =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 2)
        );

    const char* passwordHashText =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 3)
        );

    const char* displayNameText =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 4)
        );

    const int isActive =
        sqlite3_column_int(statement, 5);

    if (employeeIdText == nullptr ||
        usernameText == nullptr ||
        passwordHashText == nullptr ||
        displayNameText == nullptr) {

        sqlite3_finalize(statement);

        throw std::runtime_error(
            "User record contains invalid data."
        );
    }

    const std::string employeeId =
        employeeIdText;

    const std::string storedUsername =
        usernameText;

    const std::string storedPasswordHash =
        passwordHashText;

    const std::string displayName =
        displayNameText;

    sqlite3_finalize(statement);

    if (isActive == 0) {
        throw std::runtime_error(
            "User account is inactive."
        );
    }

    if (!security::PasswordHasher::verifyPassword(
            password,
            storedPasswordHash
        )) {
        throw std::runtime_error(
            "Invalid username or password."
        );
    }

    return AuthenticatedUser{
        userId,
        employeeId,
        storedUsername,
        displayName
    };
}

} // namespace hisab::auth