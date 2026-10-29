#pragma once

#include "Database.h"

namespace hisab::database {

class Migrations {
public:
    static void run(Database& database);

private:
    static int currentVersion(Database& database);
    static void setVersion(Database& database, int version);

    static void migrateToVersion1(Database& database);
};

} // namespace hisab::database