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

    if (currentVersion(database) == 0) {
        migrateToVersion1(database);
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
        /*
         * Version 1 establishes the initial foundation.
         *
         * The existing legacy application still owns its current
         * product/sales schema. We deliberately do not modify it here.
         *
         * New Hisab tables will be introduced through subsequent
         * migrations once the database layer has been verified.
         */

        setVersion(database, 1);

        database.commit();
    } catch (...) {
        database.rollback();
        throw;
    }
}

} // namespace hisab::database