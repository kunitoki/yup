/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

namespace
{

//==============================================================================
struct QueryCallbackData
{
    SqliteDatabase::QueryCallback fn;
};

int executeQueryCallback (void* userData, int argc, char** argv, char** columnNames)
{
    auto* data = static_cast<QueryCallbackData*> (userData);
    return data->fn (argc, argv, columnNames) ? SQLITE_OK : SQLITE_ABORT;
}

} // namespace

//==============================================================================
SqliteDatabase::SqliteDatabase() = default;
SqliteDatabase::~SqliteDatabase() = default;

bool SqliteDatabase::isOpen() const noexcept
{
    return db != nullptr;
}

const File& SqliteDatabase::getFile() const noexcept
{
    return file;
}

Result SqliteDatabase::open (const File& databaseFile, bool readOnly)
{
    close();

    const int flags = readOnly
                        ? (SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX)
                        : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX);

    sqlite3* rawDb = nullptr;

    if (sqlite3_open_v2 (databaseFile.getFullPathName().toRawUTF8(), &rawDb, flags, nullptr) != SQLITE_OK)
    {
        auto error = String::fromUTF8 (sqlite3_errmsg (rawDb));
        sqlite3_close (rawDb);
        return Result::fail (error);
    }

    db.reset (rawDb, [] (sqlite3* p)
    {
        sqlite3_close (p);
    });
    file = databaseFile;
    return Result::ok();
}

Result SqliteDatabase::openInMemory()
{
    close();

    const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;

    sqlite3* rawDb = nullptr;

    if (sqlite3_open_v2 (":memory:", &rawDb, flags, nullptr) != SQLITE_OK)
    {
        auto error = String::fromUTF8 (sqlite3_errmsg (rawDb));
        sqlite3_close (rawDb);
        return Result::fail (error);
    }

    db.reset (rawDb, [] (sqlite3* p)
    {
        sqlite3_close (p);
    });
    file = File();
    return Result::ok();
}

void SqliteDatabase::close()
{
    db.reset();
    file = File();
}

Result SqliteDatabase::vacuumToFile (const File& destFile) const
{
    if (db == nullptr)
        return Result::fail ("Database is not open");

    if (destFile.existsAsFile() && ! destFile.deleteFile())
        return Result::fail ("Can't overwrite already existing file");

    const String path = destFile.getFullPathName();
    const String sql = "VACUUM INTO '" + path.replace ("'", "''") + "'";

    char* errorMessage = nullptr;
    const int rc = sqlite3_exec (db.get(), sql.toRawUTF8(), nullptr, nullptr, &errorMessage);
    if (rc != SQLITE_OK)
    {
        auto error = errorMessage != nullptr
                       ? String::fromUTF8 (errorMessage)
                       : String::fromUTF8 (sqlite3_errmsg (db.get()));
        sqlite3_free (errorMessage);
        return Result::fail (error);
    }

    return Result::ok();
}

SqliteDatabase::Transaction SqliteDatabase::beginTransaction()
{
    return Transaction (db);
}

SqliteDatabase::Statement SqliteDatabase::prepareStatement (StringRef sql)
{
    return Statement (db, sql);
}

Result SqliteDatabase::executeQuery (StringRef sql, QueryCallback callback)
{
    if (db == nullptr)
        return Result::fail ("Database is not open");

    const bool hasCallback = callback != nullptr;
    QueryCallbackData callbackData { std::move (callback) };

    char* errorMessage = nullptr;

    const int rc = sqlite3_exec (
        db.get(),
        static_cast<const char*> (sql),
        hasCallback ? executeQueryCallback : nullptr,
        hasCallback ? static_cast<void*> (&callbackData) : nullptr,
        &errorMessage);

    if (rc != SQLITE_OK)
    {
        auto error = errorMessage != nullptr
                       ? String::fromUTF8 (errorMessage)
                       : getLastError();
        sqlite3_free (errorMessage);
        return Result::fail (error);
    }

    return Result::ok();
}

String SqliteDatabase::getLastError() const
{
    if (db == nullptr)
        return {};

    return String::fromUTF8 (sqlite3_errmsg (db.get()));
}

int SqliteDatabase::statementParamCount (const Statement& stmt) noexcept
{
    if (stmt.stmt == nullptr)
        return 0;

    return sqlite3_bind_parameter_count (stmt.stmt.get());
}

//==============================================================================
SqliteDatabase::Transaction::Transaction (std::shared_ptr<sqlite3> database)
    : db (std::move (database))
{
    if (db != nullptr)
        sqlite3_exec (db.get(), "BEGIN;", nullptr, nullptr, nullptr);
}

SqliteDatabase::Transaction::~Transaction()
{
    if (db == nullptr)
        return;

    sqlite3_exec (db.get(), shouldRollback ? "ROLLBACK;" : "COMMIT;", nullptr, nullptr, nullptr);
}

SqliteDatabase::Transaction::Transaction (Transaction&& other) noexcept
    : db (std::exchange (other.db, nullptr))
    , shouldRollback (other.shouldRollback)
{
}

SqliteDatabase::Transaction& SqliteDatabase::Transaction::operator= (Transaction&& other) noexcept
{
    if (this != &other)
    {
        db = std::exchange (other.db, nullptr);
        shouldRollback = other.shouldRollback;
    }

    return *this;
}

void SqliteDatabase::Transaction::rollback() noexcept
{
    shouldRollback = true;
}

//==============================================================================
SqliteDatabase::Statement::Statement (std::shared_ptr<sqlite3> database, StringRef sql)
    : db (std::move (database))
{
    if (db == nullptr)
        return;

    sqlite3_stmt* rawStmt = nullptr;

    if (sqlite3_prepare_v2 (db.get(), static_cast<const char*> (sql), -1, &rawStmt, nullptr) != SQLITE_OK)
        return;

    stmt.reset (rawStmt, [] (sqlite3_stmt* p)
    {
        sqlite3_finalize (p);
    });
    valid = true;
}

SqliteDatabase::Statement::~Statement() = default;

SqliteDatabase::Statement::Statement (Statement&& other) noexcept
    : db (std::exchange (other.db, nullptr))
    , stmt (std::exchange (other.stmt, nullptr))
    , valid (std::exchange (other.valid, false))
{
}

SqliteDatabase::Statement& SqliteDatabase::Statement::operator= (Statement&& other) noexcept
{
    if (this != &other)
    {
        db = std::exchange (other.db, nullptr);
        stmt = std::exchange (other.stmt, nullptr);
        valid = std::exchange (other.valid, false);
    }

    return *this;
}

bool SqliteDatabase::Statement::isValid() const noexcept
{
    return valid;
}

//==============================================================================
Result SqliteDatabase::Statement::bindNull (int column)
{
    if (stmt == nullptr)
        return Result::fail ("Statement is not valid");

    if (sqlite3_bind_null (stmt.get(), column) != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

Result SqliteDatabase::Statement::bindBool (int column, bool value)
{
    if (stmt == nullptr)
        return Result::fail ("Statement is not valid");

    if (sqlite3_bind_int (stmt.get(), column, value ? 1 : 0) != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

Result SqliteDatabase::Statement::bindInt (int column, int value)
{
    if (stmt == nullptr)
        return Result::fail ("Statement is not valid");

    if (sqlite3_bind_int (stmt.get(), column, value) != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

Result SqliteDatabase::Statement::bindInt64 (int column, int64_t value)
{
    if (stmt == nullptr)
        return Result::fail ("Statement is not valid");

    if (sqlite3_bind_int64 (stmt.get(), column, static_cast<sqlite3_int64> (value)) != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

Result SqliteDatabase::Statement::bindDouble (int column, double value)
{
    if (stmt == nullptr)
        return Result::fail ("Statement is not valid");

    if (sqlite3_bind_double (stmt.get(), column, value) != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

Result SqliteDatabase::Statement::bindText (int column, StringRef value)
{
    if (stmt == nullptr)
        return Result::fail ("Statement is not valid");

    const char* text = static_cast<const char*> (value);

    if (sqlite3_bind_text (stmt.get(), column, text, -1, SQLITE_TRANSIENT) != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

Result SqliteDatabase::Statement::bindFile (int column, const File& value)
{
    if (stmt == nullptr)
        return Result::fail ("Statement is not valid");

    const String& path = value.getFullPathName();

    if (sqlite3_bind_text (stmt.get(), column, path.toRawUTF8(), static_cast<int> (path.getNumBytesAsUTF8()), SQLITE_TRANSIENT) != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

Result SqliteDatabase::Statement::bindBlob (int column, Span<const uint8> value)
{
    if (stmt == nullptr)
        return Result::fail ("Statement is not valid");

    if (sqlite3_bind_blob (stmt.get(), column, value.data(), static_cast<int> (value.size()), SQLITE_TRANSIENT) != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

//==============================================================================
bool SqliteDatabase::Statement::columnBool (int column) const
{
    jassert (stmt != nullptr);
    return sqlite3_column_int (stmt.get(), column) != 0;
}

int SqliteDatabase::Statement::columnInt (int column) const
{
    jassert (stmt != nullptr);
    return sqlite3_column_int (stmt.get(), column);
}

int64_t SqliteDatabase::Statement::columnInt64 (int column) const
{
    jassert (stmt != nullptr);
    return static_cast<int64_t> (sqlite3_column_int64 (stmt.get(), column));
}

double SqliteDatabase::Statement::columnDouble (int column) const
{
    jassert (stmt != nullptr);
    return sqlite3_column_double (stmt.get(), column);
}

String SqliteDatabase::Statement::columnText (int column) const
{
    jassert (stmt != nullptr);

    const auto* data = sqlite3_column_text (stmt.get(), column);
    if (data == nullptr)
        return {};

    const int numBytes = sqlite3_column_bytes (stmt.get(), column);
    return String::fromUTF8 (reinterpret_cast<const char*> (data), numBytes);
}

File SqliteDatabase::Statement::columnFile (int column) const
{
    jassert (stmt != nullptr);
    return File (columnText (column));
}

Span<const uint8> SqliteDatabase::Statement::columnBlob (int column) const
{
    jassert (stmt != nullptr);

    const auto* data = sqlite3_column_blob (stmt.get(), column);
    const int numBytes = sqlite3_column_bytes (stmt.get(), column);

    return { static_cast<const uint8*> (data), static_cast<size_t> (numBytes) };
}

//==============================================================================
std::optional<bool> SqliteDatabase::Statement::columnOptionalBool (int column) const
{
    if (stmt == nullptr || sqlite3_column_type (stmt.get(), column) == SQLITE_NULL)
        return std::nullopt;

    return sqlite3_column_int (stmt.get(), column) != 0;
}

std::optional<int> SqliteDatabase::Statement::columnOptionalInt (int column) const
{
    if (stmt == nullptr || sqlite3_column_type (stmt.get(), column) == SQLITE_NULL)
        return std::nullopt;

    return sqlite3_column_int (stmt.get(), column);
}

std::optional<int64_t> SqliteDatabase::Statement::columnOptionalInt64 (int column) const
{
    if (stmt == nullptr || sqlite3_column_type (stmt.get(), column) == SQLITE_NULL)
        return std::nullopt;

    return static_cast<int64_t> (sqlite3_column_int64 (stmt.get(), column));
}

std::optional<double> SqliteDatabase::Statement::columnOptionalDouble (int column) const
{
    if (stmt == nullptr || sqlite3_column_type (stmt.get(), column) == SQLITE_NULL)
        return std::nullopt;

    return sqlite3_column_double (stmt.get(), column);
}

std::optional<String> SqliteDatabase::Statement::columnOptionalText (int column) const
{
    if (stmt == nullptr || sqlite3_column_type (stmt.get(), column) == SQLITE_NULL)
        return std::nullopt;

    const auto* data = sqlite3_column_text (stmt.get(), column);
    if (data == nullptr)
        return std::nullopt;

    const int numBytes = sqlite3_column_bytes (stmt.get(), column);
    return String::fromUTF8 (reinterpret_cast<const char*> (data), numBytes);
}

std::optional<File> SqliteDatabase::Statement::columnOptionalFile (int column) const
{
    auto text = columnOptionalText (column);

    if (! text.has_value())
        return std::nullopt;

    return File (*text);
}

std::optional<Span<const uint8>> SqliteDatabase::Statement::columnOptionalBlob (int column) const
{
    if (stmt == nullptr || sqlite3_column_type (stmt.get(), column) == SQLITE_NULL)
        return std::nullopt;

    const auto* data = sqlite3_column_blob (stmt.get(), column);
    const int numBytes = sqlite3_column_bytes (stmt.get(), column);

    return Span<const uint8> { static_cast<const uint8*> (data), static_cast<size_t> (numBytes) };
}

//==============================================================================
Result SqliteDatabase::Statement::readBlob (StringRef tableName,
                                            StringRef fieldName,
                                            int64_t rowId,
                                            std::function<void*(int)> sizeCallback)
{
    if (db == nullptr)
        return Result::fail ("Statement has no database connection");

    sqlite3_blob* blob = nullptr;

    const int openResult = sqlite3_blob_open (
        db.get(),
        "main",
        static_cast<const char*> (tableName),
        static_cast<const char*> (fieldName),
        static_cast<sqlite3_int64> (rowId),
        0,
        &blob);

    if (openResult != SQLITE_OK)
    {
        auto error = String::fromUTF8 (sqlite3_errmsg (db.get()));
        if (blob != nullptr)
            sqlite3_blob_close (blob);
        return Result::fail (error);
    }

    const int numBytes = sqlite3_blob_bytes (blob);
    void* buffer = sizeCallback (numBytes);

    const int readResult = sqlite3_blob_read (blob, buffer, numBytes, 0);
    sqlite3_blob_close (blob);

    if (readResult != SQLITE_OK)
        return Result::fail (String::fromUTF8 (sqlite3_errmsg (db.get())));

    return Result::ok();
}

//==============================================================================
int SqliteDatabase::Statement::step()
{
    if (stmt == nullptr)
        return SQLITE_MISUSE;

    return sqlite3_step (stmt.get());
}

void SqliteDatabase::Statement::reset()
{
    if (stmt == nullptr)
        return;

    sqlite3_clear_bindings (stmt.get());
    sqlite3_reset (stmt.get());
}

} // namespace yup
