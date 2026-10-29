#include "Database.h"

#include <stdexcept>
#include <string>

namespace hisab::database {

Database::Database(const std::string& path)
    : path_(path) {
}

Database::~Database() {
    close();
}

void Database::open() {
    if (isOpen()) {
        return;
    }

    sqlite3* connection = nullptr;

    const int result = sqlite3_open(path_.c_str(), &connection);

    if (result != SQLITE_OK) {
        std::string message = "Failed to open SQLite database";

        if (connection != nullptr) {
            message += ": ";
            message += sqlite3_errmsg(connection);
            sqlite3_close(connection);
        }

        throw std::runtime_error(message);
    }

    db_ = connection;

    try {
        enableForeignKeys();
    } catch (...) {
        close();
        throw;
    }
}

void Database::close() {
    if (db_ == nullptr) {
        return;
    }

    const int result = sqlite3_close(db_);

    if (result != SQLITE_OK) {
        throw std::runtime_error(
            "Failed to close SQLite database: " +
            std::string(sqlite3_errmsg(db_))
        );
    }

    db_ = nullptr;
}

bool Database::isOpen() const noexcept {
    return db_ != nullptr;
}

sqlite3* Database::handle() noexcept {
    return db_;
}

const std::string& Database::path() const noexcept {
    return path_;
}

void Database::execute(const std::string& sql) {
    if (!isOpen()) {
        throw std::runtime_error(
            "Cannot execute SQL: database is not open"
        );
    }

    char* errorMessage = nullptr;

    const int result = sqlite3_exec(
        db_,
        sql.c_str(),
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        std::string message = "SQLite execution failed";

        if (errorMessage != nullptr) {
            message += ": ";
            message += errorMessage;
            sqlite3_free(errorMessage);
        }

        throw std::runtime_error(message);
    }
}

void Database::beginTransaction() {
    execute("BEGIN TRANSACTION;");
}

void Database::commit() {
    execute("COMMIT;");
}

void Database::rollback() {
    execute("ROLLBACK;");
}

void Database::enableForeignKeys() {
    execute("PRAGMA foreign_keys = ON;");
}

} // namespace hisab::database