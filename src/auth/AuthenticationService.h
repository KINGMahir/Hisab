#pragma once

#include <string>

#include "database/Database.h"

namespace hisab::auth {

struct AuthenticatedUser {
    int id;
    std::string employeeId;
    std::string username;
    std::string displayName;
};

class AuthenticationService {
public:
    explicit AuthenticationService(
        database::Database& database
    );

    AuthenticatedUser login(
        const std::string& username,
        const std::string& password
    );

private:
    database::Database& database_;
};

} // namespace hisab::auth