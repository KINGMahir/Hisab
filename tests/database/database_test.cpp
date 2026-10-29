#include "database/Database.h"
#include "database/Migrations.h"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        hisab::database::Database database("hisab_test.db");

        std::cout << "Opening database...\n";
        database.open();

        if (!database.isOpen()) {
            throw std::runtime_error("Database did not open.");
        }

        std::cout << "Database opened successfully.\n";

        std::cout << "Running migrations...\n";
        hisab::database::Migrations::run(database);

        std::cout << "Migrations completed successfully.\n";

        database.execute(
            "CREATE TABLE IF NOT EXISTS test_transaction ("
            "    id INTEGER PRIMARY KEY,"
            "    name TEXT NOT NULL"
            ");"
        );

        std::cout << "Testing transaction...\n";

        database.beginTransaction();

        database.execute(
            "INSERT INTO test_transaction (name) "
            "VALUES ('Hisab Database Test');"
        );

        database.commit();

        std::cout << "Transaction committed successfully.\n";

        database.close();

        std::cout << "Database closed successfully.\n";
        std::cout << "\nDatabase runtime test PASSED.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "\nDatabase runtime test FAILED:\n";
        std::cerr << error.what() << '\n';

        return 1;
    }
}