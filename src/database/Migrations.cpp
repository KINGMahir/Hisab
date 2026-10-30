#include "Migrations.h"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace hisab::database {

void Migrations::run(Database& database) {
    if (!database.isOpen()) {
        throw std::runtime_error(
            "Cannot run migrations: database is not open"
        );
    }

    database.execute(
        "CREATE TABLE IF NOT EXISTS schema_version ("
        "    version INTEGER NOT NULL"
        ");"
    );

    int version = currentVersion(database);

    if (version < 1) {
        migrateToVersion1(database);
        version = 1;
    }

    if (version < 2) {
        migrateToVersion2(database);
        version = 2;
    }

    if (version < 3) {
        migrateToVersion3(database);
        version = 3;
    }
}

int Migrations::currentVersion(Database& database) {
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "SELECT version "
        "FROM schema_version "
        "LIMIT 1;";

    const int prepareResult = sqlite3_prepare_v2(
        database.handle(),
        sql,
        -1,
        &statement,
        nullptr
    );

    if (prepareResult != SQLITE_OK) {
        throw std::runtime_error(
            "Failed to prepare schema version query: " +
            std::string(sqlite3_errmsg(database.handle()))
        );
    }

    const int stepResult = sqlite3_step(statement);

    int version = 0;

    if (stepResult == SQLITE_ROW) {
        version = sqlite3_column_int(statement, 0);
    } else if (stepResult != SQLITE_DONE) {
        sqlite3_finalize(statement);

        throw std::runtime_error(
            "Failed to read schema version: " +
            std::string(sqlite3_errmsg(database.handle()))
        );
    }

    sqlite3_finalize(statement);

    return version;
}

void Migrations::setVersion(Database& database, int version) {
    database.execute("DELETE FROM schema_version;");

    const std::string sql =
        "INSERT INTO schema_version (version) VALUES (" +
        std::to_string(version) +
        ");";

    database.execute(sql);
}

void Migrations::migrateToVersion1(Database& database) {
    database.beginTransaction();

    try {
        setVersion(database, 1);

        database.commit();
    } catch (...) {
        database.rollback();
        throw;
    }
}

void Migrations::migrateToVersion2(Database& database) {
    database.beginTransaction();

    try {
        database.execute(
            "CREATE TABLE users ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    employee_id TEXT NOT NULL UNIQUE,"
            "    username TEXT NOT NULL UNIQUE,"
            "    password_hash TEXT NOT NULL,"
            "    display_name TEXT NOT NULL,"
            "    is_active INTEGER NOT NULL DEFAULT 1,"
            "    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");"
        );

        database.execute(
            "CREATE INDEX idx_users_employee_id "
            "ON users(employee_id);"
        );

        database.execute(
            "CREATE INDEX idx_users_username "
            "ON users(username);"
        );

        setVersion(database, 2);

        database.commit();
    } catch (...) {
        database.rollback();
        throw;
    }
}

void Migrations::migrateToVersion3(Database& database) {
    database.beginTransaction();

    try {
        database.execute(
            "CREATE TABLE roles ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    name TEXT NOT NULL UNIQUE,"
            "    description TEXT,"
            "    is_active INTEGER NOT NULL DEFAULT 1,"
            "    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");"
        );

        database.execute(
            "CREATE TABLE permissions ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    name TEXT NOT NULL UNIQUE,"
            "    description TEXT,"
            "    is_active INTEGER NOT NULL DEFAULT 1,"
            "    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");"
        );

        database.execute(
            "CREATE TABLE user_roles ("
            "    user_id INTEGER NOT NULL,"
            "    role_id INTEGER NOT NULL,"
            "    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    PRIMARY KEY (user_id, role_id),"
            "    FOREIGN KEY (user_id) REFERENCES users(id) "
            "        ON DELETE CASCADE,"
            "    FOREIGN KEY (role_id) REFERENCES roles(id) "
            "        ON DELETE CASCADE"
            ");"
        );

        database.execute(
            "CREATE TABLE role_permissions ("
            "    role_id INTEGER NOT NULL,"
            "    permission_id INTEGER NOT NULL,"
            "    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    PRIMARY KEY (role_id, permission_id),"
            "    FOREIGN KEY (role_id) REFERENCES roles(id) "
            "        ON DELETE CASCADE,"
            "    FOREIGN KEY (permission_id) REFERENCES permissions(id) "
            "        ON DELETE CASCADE"
            ");"
        );

        database.execute(
            "CREATE INDEX idx_user_roles_user_id "
            "ON user_roles(user_id);"
        );

        database.execute(
            "CREATE INDEX idx_user_roles_role_id "
            "ON user_roles(role_id);"
        );

        database.execute(
            "CREATE INDEX idx_role_permissions_role_id "
            "ON role_permissions(role_id);"
        );

        database.execute(
            "CREATE INDEX idx_role_permissions_permission_id "
            "ON role_permissions(permission_id);"
        );

        setVersion(database, 3);

        database.commit();
    }
    catch (...) {
        database.rollback();
        throw;
    }
}

} // namespace hisab::database