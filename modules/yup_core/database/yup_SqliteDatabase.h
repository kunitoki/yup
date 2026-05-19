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

//==============================================================================
/**
    A lightweight wrapper around an SQLite database connection.

    Provides RAII transaction management, compiled prepared statements with
    type-safe parameter binding and column access, and a simple one-shot query
    execution interface.

    This class is only available when the sqlite3_library module is present
    (i.e. when YUP_MODULE_AVAILABLE_sqlite3_library is defined).

    Typical usage:

    @code
    SqliteDatabase db;
    if (db.open (File::getSpecialLocation (File::tempDirectory).getChildFile ("my.db")))
    {
        db.executeQuery ("CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT)");

        {
            auto tx = db.beginTransaction();
            // One-shot: compile + bind + ready to step, all in one call.
            auto result = db.prepareStatement ("INSERT OR REPLACE INTO kv VALUES (?, ?)",
                                               "greeting", "hello");
            if (result.wasOk())
                result.getReference().step();
        }  // transaction commits here

        auto result = db.prepareStatement ("SELECT value FROM kv WHERE key = ?", "greeting");
        if (result.wasOk())
        {
            auto& sel = result.getReference();
            if (sel.step() == SQLITE_ROW)
                DBG (sel.columnText (0));
        }
    }
    @endcode

    @see SqliteDatabase::Transaction, SqliteDatabase::Statement

    @tags{Core}
*/
class YUP_API SqliteDatabase
{
public:
    //==============================================================================
    /** Callback type used by executeQuery().

        Receives the column count, value array and column-name array for each row.
        Return true to continue iterating, false to stop.
    */
    using QueryCallback = std::function<bool (int, char**, char**)>;

    //==============================================================================
    /**
        RAII guard for a database transaction.

        The transaction is committed automatically when this object is destroyed.
        Call rollback() before destruction to roll back instead.
    */
    class YUP_API Transaction
    {
    public:
        /** Commits (or rolls back) the transaction. */
        ~Transaction();

        Transaction (const Transaction&) = delete;
        Transaction& operator= (const Transaction&) = delete;

        Transaction (Transaction&&) noexcept;
        Transaction& operator= (Transaction&&) noexcept;

        /** Marks this transaction to be rolled back on destruction instead of committed. */
        void rollback() noexcept;

    private:
        friend class SqliteDatabase;

        explicit Transaction (std::shared_ptr<sqlite3> database);

        std::shared_ptr<sqlite3> db;
        bool shouldRollback = false;
    };

    //==============================================================================
    /**
        A compiled, reusable prepared statement.

        Bind-parameter indices are 1-based; column indices are 0-based — both
        follow standard SQLite conventions.

        The raw data pointers returned by columnBlob() and the Span returned by
        columnOptionalBlob() point directly into SQLite's internal buffers and are
        only valid until the next call to step() or reset().  Use MemoryBlock or
        copy the data if you need it to outlive a single iteration step.
    */
    class YUP_API Statement
    {
    public:
        //==============================================================================
        /** Finalizes the underlying statement. */
        ~Statement();

        Statement (const Statement&) = delete;
        Statement& operator= (const Statement&) = delete;

        Statement (Statement&&) noexcept;
        Statement& operator= (Statement&&) noexcept;

        //==============================================================================
        /** Returns true if the statement was compiled successfully. */
        bool isValid() const noexcept;

        //==============================================================================
        /** Binds SQL NULL to a parameter. */
        Result bindNull (int column);

        /** Binds a bool value (stored as integer 0 or 1). */
        Result bindBool (int column, bool value);

        /** Binds a 32-bit signed integer. */
        Result bindInt (int column, int value);

        /** Binds a 64-bit signed integer. */
        Result bindInt64 (int column, int64_t value);

        /** Binds a double-precision floating-point value. */
        Result bindDouble (int column, double value);

        /** Binds a UTF-8 text value (SQLite makes an internal copy). */
        Result bindText (int column, StringRef value);

        /** Binds a File's full path as UTF-8 text (SQLite makes an internal copy). */
        Result bindFile (int column, const File& value);

        /** Binds raw binary data (SQLite makes an internal copy). */
        Result bindBlob (int column, Span<const uint8> value);

        //==============================================================================
        /** @name Column readers (0-based indices)

            These methods assert in debug builds when the statement is invalid.
            Use the optional variants if you need to detect SQL NULL.
        */
        ///@{
        bool columnBool (int column) const;
        int columnInt (int column) const;
        int64_t columnInt64 (int column) const;
        double columnDouble (int column) const;

        /** Returns a copy of the column's UTF-8 text as a String. */
        String columnText (int column) const;

        /** Interprets the column's text as a file path and returns a File. */
        File columnFile (int column) const;

        /** Returns a non-owning view of the raw blob bytes.
            The data is only valid until the next step() or reset() call.
        */
        Span<const uint8> columnBlob (int column) const;
        ///@}

        //==============================================================================
        /** @name Nullable column readers (0-based indices)

            Return std::nullopt when the column contains SQL NULL.
        */
        ///@{
        std::optional<bool> columnOptionalBool (int column) const;
        std::optional<int> columnOptionalInt (int column) const;
        std::optional<int64_t> columnOptionalInt64 (int column) const;
        std::optional<double> columnOptionalDouble (int column) const;
        std::optional<String> columnOptionalText (int column) const;
        std::optional<File> columnOptionalFile (int column) const;
        std::optional<Span<const uint8>> columnOptionalBlob (int column) const;
        ///@}

        //==============================================================================
        /**
            Reads a BLOB column using SQLite's incremental I/O interface.

            This avoids loading the entire blob into memory before copying it.
            The @p sizeCallback is invoked with the blob size in bytes; it must
            return a writable buffer of exactly that size.  The data is then
            read directly into that buffer.

            @param tableName    The table that contains the blob column.
            @param fieldName    The column name.
            @param rowId        The rowid of the target row.
            @param sizeCallback Called with the byte count; returns the target buffer.

            @returns Result::ok() on success, Result::fail() with an error message otherwise.
        */
        Result readBlob (StringRef tableName,
                         StringRef fieldName,
                         int64_t rowId,
                         std::function<void*(int)> sizeCallback);

        //==============================================================================
        /** Advances the statement one step.
            Returns SQLITE_ROW when a result row is available, SQLITE_DONE when
            execution is complete, or an error code otherwise.
        */
        int step();

        /** Clears all bound parameters and resets the statement for reuse. */
        void reset();

    private:
        friend class SqliteDatabase;

        Statement (std::shared_ptr<sqlite3> database, StringRef sql);

        std::shared_ptr<sqlite3> db;
        std::shared_ptr<sqlite3_stmt> stmt;
        bool valid = false;
    };

    //==============================================================================
    /** Construct a closed sqlite database. */
    SqliteDatabase();

    /** Close a sqlite database. */
    ~SqliteDatabase();

    SqliteDatabase (const SqliteDatabase& other) = default;
    SqliteDatabase& operator= (const SqliteDatabase& other) = default;
    SqliteDatabase (SqliteDatabase&& other) = default;
    SqliteDatabase& operator= (SqliteDatabase&& other) = default;

    //==============================================================================
    /** Returns true if the database is currently open. */
    bool isOpen() const noexcept;

    /** Returns the File that was used to open the database.
        Returns a default-constructed (empty) File when the database is closed.
    */
    const File& getFile() const noexcept;

    //==============================================================================
    /**
        Opens the database at the given file path.

        If @p readOnly is false and the file does not exist it is created.

        @returns Result::ok() on success, Result::fail() with an error message otherwise.
    */
    Result open (const File& databaseFile, bool readOnly = false);

    /**
        Opens a private, temporary in-memory database.

        The database exists only for the lifetime of this connection and is
        not backed by any file.  getFile() returns an empty File for
        in-memory databases.

        @returns Result::ok() on success, Result::fail() with an error message otherwise.
    */
    Result openInMemory();

    /** Closes the database connection and resets the file path. */
    void close();

    //==============================================================================
    /**
        Creates a compacted copy of the database at the given file path.

        Uses SQLite's @c VACUUM INTO statement, which writes a defragmented
        snapshot of the current database (including in-memory databases) to a
        new file without disturbing the source connection.

        Requires SQLite 3.27.0 or later.

        @param file  Destination path.  The file must not already be open as a
                     SQLite database, but it may already exist (it will be
                     overwritten).

        @returns Result::ok() on success, Result::fail() with an error message otherwise.
    */
    Result vacuumToFile (const File& file) const;

    //==============================================================================
    /** Begins a new transaction and returns an RAII guard.
        The guard commits on destruction unless rollback() is called first.
    */
    Transaction beginTransaction();

    //==============================================================================
    /** Compiles a SQL statement and returns a reusable Statement object.
        Check Statement::isValid() to verify compilation succeeded.
    */
    Statement prepareStatement (StringRef sql);

    /**
        Compiles a SQL statement and immediately binds @p arg and @p rest to its
        parameters (1-based, in order).

        Supported argument types: bool, any integral, any floating-point,
        StringRef / String / const char*, File, Span<const uint8>,
        nullptr_t / nullopt_t (binds SQL NULL).

        @returns ResultValue::ok() holding the ready-to-step Statement on success,
                 or ResultValue::fail() with an error message if compilation or
                 any binding fails.
    */
    template <class Arg, class... Rest>
    ResultValue<Statement> prepareStatement (StringRef sql, Arg&& arg, Rest&&... rest)
    {
        constexpr int numArgs = 1 + static_cast<int> (sizeof...(Rest));

        auto stmt = prepareStatement (sql);

        if (! stmt.isValid())
            return makeResultValueFail (getLastError());

        const int expectedParams = statementParamCount (stmt);
        if (numArgs != expectedParams)
            return makeResultValueFail (
                "Expected " + String (expectedParams) + " bind parameter(s), got " + String (numArgs));

        if (auto r = bindArgPack (stmt, 1, std::forward<Arg> (arg), std::forward<Rest> (rest)...); r.failed())
            return makeResultValueFail (r.getErrorMessage());

        return makeResultValueOk (std::move (stmt));
    }

    //==============================================================================
    /**
        Executes a SQL string immediately, invoking @p callback for each result row.

        @p callback may be nullptr when no results are expected (e.g. DDL or DML).
        @returns Result::ok() on success, Result::fail() with an error message otherwise.
    */
    Result executeQuery (StringRef sql, QueryCallback callback = nullptr);

    //==============================================================================
    /** Returns the most recent error message from the underlying SQLite connection.
        Returns an empty string when the database is not open.
    */
    String getLastError() const;

private:
    static int statementParamCount (const Statement& stmt) noexcept;

    template <class T>
    static constexpr bool unsupportedBindType = false;

    template <class T>
    static Result bindSingleArg (Statement& stmt, int column, T&& value)
    {
        using V = std::decay_t<T>;

        if constexpr (std::is_same_v<V, std::nullptr_t> || std::is_same_v<V, std::nullopt_t>)
            return stmt.bindNull (column);
        else if constexpr (std::is_same_v<V, bool>)
            return stmt.bindBool (column, value);
        else if constexpr (std::is_integral_v<V> && sizeof (V) <= 4)
            return stmt.bindInt (column, static_cast<int> (value));
        else if constexpr (std::is_integral_v<V>)
            return stmt.bindInt64 (column, static_cast<int64_t> (value));
        else if constexpr (std::is_floating_point_v<V>)
            return stmt.bindDouble (column, static_cast<double> (value));
        else if constexpr (std::is_same_v<V, File>)
            return stmt.bindFile (column, value);
        else if constexpr (std::is_constructible_v<StringRef, V>)
            return stmt.bindText (column, StringRef (std::forward<T> (value)));
        else if constexpr (std::is_constructible_v<Span<const uint8>, V>)
            return stmt.bindBlob (column, Span<const uint8> (std::forward<T> (value)));
        else
            static_assert (unsupportedBindType<T>,
                           "SqliteDatabase::prepareStatement: no SQLite binding for this argument type");
    }

    template <class Head, class... Tail>
    static Result bindArgPack (Statement& stmt, int column, Head&& head, Tail&&... tail)
    {
        if (auto r = bindSingleArg (stmt, column, std::forward<Head> (head)); r.failed())
            return r;

        if constexpr (sizeof...(Tail) > 0)
            return bindArgPack (stmt, column + 1, std::forward<Tail> (tail)...);

        return Result::ok();
    }

    std::shared_ptr<sqlite3> db;
    File file;

    YUP_LEAK_DETECTOR (SqliteDatabase)
};

} // namespace yup
