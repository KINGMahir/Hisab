#pragma once

#include <sqlite3.h>

#include <string>

namespace hisab::database {

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;

    void open();
    void close();

    bool isOpen() const noexcept;

    sqlite3* handle() noexcept;
    const std::string& path() const noexcept;

    void execute(const std::string& sql);

    void beginTransaction();
    void commit();
    void rollback();

private:
    std::string path_;
    sqlite3* db_ = nullptr;

    void enableForeignKeys();
};

} // namespace hisab::database