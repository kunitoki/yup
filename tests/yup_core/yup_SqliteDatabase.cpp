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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

#if YUP_MODULE_AVAILABLE_sqlite3_library

//==============================================================================
// Helper to open an in-memory database and assert success.
static SqliteDatabase openMemory()
{
    SqliteDatabase db;
    EXPECT_TRUE (db.openInMemory());
    return db;
}

//==============================================================================
class SqliteDatabaseTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        db = openMemory();
    }

    SqliteDatabase db;
};

//==============================================================================
// Construction & lifecycle
//==============================================================================

TEST (SqliteDatabaseLifecycleTests, DefaultConstructionIsNotOpen)
{
    SqliteDatabase db;
    EXPECT_FALSE (db.isOpen());
    EXPECT_TRUE (db.getFile().getFullPathName().isEmpty());
}

TEST (SqliteDatabaseLifecycleTests, OpenInMemorySucceeds)
{
    SqliteDatabase db;
    EXPECT_TRUE (db.openInMemory());
    EXPECT_TRUE (db.isOpen());
    EXPECT_TRUE (db.getFile().getFullPathName().isEmpty());
}

TEST (SqliteDatabaseLifecycleTests, CloseResetsState)
{
    SqliteDatabase db;
    db.openInMemory();
    db.close();
    EXPECT_FALSE (db.isOpen());
    EXPECT_TRUE (db.getFile().getFullPathName().isEmpty());
}

TEST (SqliteDatabaseLifecycleTests, OpenAgainClosesExistingConnection)
{
    SqliteDatabase db;
    EXPECT_TRUE (db.openInMemory());
    EXPECT_TRUE (db.openInMemory()); // should close the first and reopen
    EXPECT_TRUE (db.isOpen());
}

TEST (SqliteDatabaseLifecycleTests, OpenReadOnlyNonExistentFileFails)
{
    SqliteDatabase db;
    File nonExistent (File::getSpecialLocation (File::tempDirectory)
                          .getChildFile ("yup_no_such_db_" + String::toHexString (Random::getSystemRandom().nextInt()) + ".db"));

    EXPECT_FALSE (db.open (nonExistent, true));
    EXPECT_FALSE (db.isOpen());
}

TEST (SqliteDatabaseLifecycleTests, OpenFileBasedDatabase)
{
    File tempDb = File::getSpecialLocation (File::tempDirectory)
                      .getChildFile ("yup_sqlite_test_" + String::toHexString (Random::getSystemRandom().nextInt()) + ".db");

    {
        SqliteDatabase db;
        EXPECT_TRUE (db.open (tempDb));
        EXPECT_TRUE (db.executeQuery ("CREATE TABLE t (v INTEGER)"));
        EXPECT_TRUE (db.executeQuery ("INSERT INTO t VALUES (99)"));
    }

    // Reopen and verify data persisted.
    {
        SqliteDatabase db;
        EXPECT_TRUE (db.open (tempDb));
        int value = 0;
        db.executeQuery ("SELECT v FROM t", [&] (int, char** argv, char**)
        {
            value = std::stoi (argv[0]);
            return true;
        });
        EXPECT_EQ (99, value);
    }

    tempDb.deleteFile();
}

//==============================================================================
// executeQuery
//==============================================================================

TEST_F (SqliteDatabaseTests, ExecuteQueryCreateTable)
{
    EXPECT_TRUE (db.executeQuery ("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)"));
}

TEST_F (SqliteDatabaseTests, ExecuteQueryBadSqlReturnsFalse)
{
    EXPECT_FALSE (db.executeQuery ("THIS IS NOT SQL AT ALL"));
}

TEST_F (SqliteDatabaseTests, ExecuteQueryOnClosedDatabaseReturnsFalse)
{
    SqliteDatabase closed;
    EXPECT_FALSE (closed.executeQuery ("CREATE TABLE t (x INTEGER)"));
}

TEST_F (SqliteDatabaseTests, ExecuteQueryWithCallbackReceivesRows)
{
    db.executeQuery ("CREATE TABLE kv (key TEXT, value TEXT)");
    db.executeQuery ("INSERT INTO kv VALUES ('a', 'apple')");
    db.executeQuery ("INSERT INTO kv VALUES ('b', 'banana')");

    std::vector<std::pair<std::string, std::string>> rows;
    db.executeQuery ("SELECT key, value FROM kv ORDER BY key",
                     [&] (int argc, char** argv, char** /*colNames*/)
    {
        EXPECT_EQ (2, argc);
        rows.emplace_back (argv[0], argv[1]);
        return true;
    });

    ASSERT_EQ (2u, rows.size());
    EXPECT_EQ ("a", rows[0].first);
    EXPECT_EQ ("apple", rows[0].second);
    EXPECT_EQ ("b", rows[1].first);
    EXPECT_EQ ("banana", rows[1].second);
}

TEST_F (SqliteDatabaseTests, ExecuteQueryCallbackReturnFalseAbortsEarly)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (1)");
    db.executeQuery ("INSERT INTO t VALUES (2)");
    db.executeQuery ("INSERT INTO t VALUES (3)");

    int callCount = 0;
    db.executeQuery ("SELECT v FROM t ORDER BY v",
                     [&] (int, char**, char**)
    {
        ++callCount;
        return false; // stop after first row
    });

    EXPECT_EQ (1, callCount);
}

//==============================================================================
// getLastError
//==============================================================================

TEST (SqliteDatabaseErrorTests, GetLastErrorOnClosedDatabaseReturnsEmpty)
{
    SqliteDatabase db;
    EXPECT_TRUE (db.getLastError().isEmpty());
}

TEST_F (SqliteDatabaseTests, GetLastErrorAfterBadSqlIsNotEmpty)
{
    db.executeQuery ("COMPLETELY WRONG");
    EXPECT_TRUE (db.getLastError().isNotEmpty());
}

//==============================================================================
// prepareStatement / Statement::isValid
//==============================================================================

TEST_F (SqliteDatabaseTests, PrepareValidStatementIsValid)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    auto stmt = db.prepareStatement ("SELECT v FROM t");
    EXPECT_TRUE (stmt.isValid());
}

TEST_F (SqliteDatabaseTests, PrepareBadSqlIsNotValid)
{
    auto stmt = db.prepareStatement ("GARBAGE QUERY !!!");
    EXPECT_FALSE (stmt.isValid());
}

TEST (SqliteDatabaseStatementTests, PrepareOnNullDatabaseIsNotValid)
{
    SqliteDatabase closed;
    auto stmt = closed.prepareStatement ("SELECT 1");
    EXPECT_FALSE (stmt.isValid());
}

//==============================================================================
// Statement move semantics
//==============================================================================

TEST_F (SqliteDatabaseTests, StatementMoveConstructor)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    auto original = db.prepareStatement ("SELECT v FROM t");
    EXPECT_TRUE (original.isValid());

    auto moved = std::move (original);
    EXPECT_TRUE (moved.isValid());
    EXPECT_FALSE (original.isValid());
}

TEST_F (SqliteDatabaseTests, StatementMoveAssignment)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    auto stmtA = db.prepareStatement ("SELECT v FROM t");
    SqliteDatabase::Statement stmtB (std::move (stmtA));
    EXPECT_FALSE (stmtA.isValid());
    EXPECT_TRUE (stmtB.isValid());
}

//==============================================================================
// step / reset
//==============================================================================

TEST_F (SqliteDatabaseTests, StepInsertReturnsDone)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    auto stmt = db.prepareStatement ("INSERT INTO t VALUES (1)");
    EXPECT_EQ (SQLITE_DONE, stmt.step());
}

TEST_F (SqliteDatabaseTests, StepSelectReturnsRow)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (42)");
    auto stmt = db.prepareStatement ("SELECT v FROM t");
    EXPECT_EQ (SQLITE_ROW, stmt.step());
}

TEST_F (SqliteDatabaseTests, StepExhaustedReturnsDone)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    auto stmt = db.prepareStatement ("SELECT v FROM t");
    EXPECT_EQ (SQLITE_DONE, stmt.step());
}

TEST_F (SqliteDatabaseTests, StepOnInvalidStatementReturnsMisuse)
{
    SqliteDatabase::Statement invalid (db.prepareStatement ("BROKEN !!"));
    EXPECT_FALSE (invalid.isValid());
    EXPECT_EQ (SQLITE_MISUSE, invalid.step());
}

TEST_F (SqliteDatabaseTests, ResetAllowsReuse)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (7)");
    db.executeQuery ("INSERT INTO t VALUES (8)");

    auto stmt = db.prepareStatement ("SELECT v FROM t ORDER BY v");
    EXPECT_EQ (SQLITE_ROW, stmt.step());
    EXPECT_EQ (7, stmt.columnInt (0));
    EXPECT_EQ (SQLITE_ROW, stmt.step());
    EXPECT_EQ (8, stmt.columnInt (0));

    stmt.reset();

    // After reset, step from the beginning again.
    EXPECT_EQ (SQLITE_ROW, stmt.step());
    EXPECT_EQ (7, stmt.columnInt (0));
}

TEST_F (SqliteDatabaseTests, IterateMultipleRowsWithStep)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    for (int i = 1; i <= 10; ++i)
        db.executeQuery ("INSERT INTO t VALUES (" + String (i) + ")");

    auto stmt = db.prepareStatement ("SELECT v FROM t ORDER BY v");
    int expected = 1;
    while (stmt.step() == SQLITE_ROW)
        EXPECT_EQ (expected++, stmt.columnInt (0));

    EXPECT_EQ (11, expected);
}

//==============================================================================
// Bind / Column — bool
//==============================================================================

TEST_F (SqliteDatabaseTests, BindAndColumnBool)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindBool (1, true);
        ins.step();
        ins.reset();
        ins.bindBool (1, false);
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t ORDER BY v");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnBool (0));
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_TRUE (sel.columnBool (0));
}

//==============================================================================
// Bind / Column — int
//==============================================================================

TEST_F (SqliteDatabaseTests, BindAndColumnInt)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        for (int v : { -100, 0, 1, std::numeric_limits<int>::max() })
        {
            ins.bindInt (1, v);
            ins.step();
            ins.reset();
        }
    }

    auto sel = db.prepareStatement ("SELECT v FROM t ORDER BY v");
    std::vector<int> results;
    while (sel.step() == SQLITE_ROW)
        results.push_back (sel.columnInt (0));

    ASSERT_EQ (4u, results.size());
    EXPECT_EQ (-100, results[0]);
    EXPECT_EQ (0, results[1]);
    EXPECT_EQ (1, results[2]);
    EXPECT_EQ (std::numeric_limits<int>::max(), results[3]);
}

//==============================================================================
// Bind / Column — int64
//==============================================================================

TEST_F (SqliteDatabaseTests, BindAndColumnInt64)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    const int64_t large = static_cast<int64_t> (std::numeric_limits<int>::max()) * 4LL + 1;
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindInt64 (1, large);
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (large, sel.columnInt64 (0));
}

//==============================================================================
// Bind / Column — double
//==============================================================================

TEST_F (SqliteDatabaseTests, BindAndColumnDouble)
{
    db.executeQuery ("CREATE TABLE t (v REAL)");
    const double pi = 3.141592653589793;
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindDouble (1, pi);
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_DOUBLE_EQ (pi, sel.columnDouble (0));
}

//==============================================================================
// Bind / Column — text
//==============================================================================

TEST_F (SqliteDatabaseTests, BindAndColumnText)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindText (1, "hello world");
        ins.step();
        ins.reset();
        ins.bindText (1, ""); // empty string
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t ORDER BY v");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ ("", sel.columnText (0));
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ ("hello world", sel.columnText (0));
}

TEST_F (SqliteDatabaseTests, BindAndColumnTextUnicode)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    const String unicode = String::fromUTF8 ("\xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF"); // "こんにちは"
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindText (1, unicode);
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (unicode, sel.columnText (0));
}

//==============================================================================
// Bind / Column — File
//==============================================================================

TEST_F (SqliteDatabaseTests, BindAndColumnFile)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    const File f ("/usr/local/bin/sqlite3");
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindFile (1, f);
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (f.getFullPathName(), sel.columnFile (0).getFullPathName());
    EXPECT_EQ (f.getFullPathName(), sel.columnText (0));
}

//==============================================================================
// Bind / Column — blob
//==============================================================================

TEST_F (SqliteDatabaseTests, BindAndColumnBlob)
{
    db.executeQuery ("CREATE TABLE t (v BLOB)");
    const std::vector<uint8> blobData = { 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0xFF };
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindBlob (1, Span<const uint8> (blobData.data(), blobData.size()));
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());

    const auto blob = sel.columnBlob (0);
    ASSERT_EQ (blobData.size(), blob.size());
    EXPECT_EQ (0, std::memcmp (blobData.data(), blob.data(), blobData.size()));
}

TEST_F (SqliteDatabaseTests, BindEmptyBlob)
{
    db.executeQuery ("CREATE TABLE t (v BLOB)");
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindBlob (1, Span<const uint8>());
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    // An empty blob may come back as NULL or as a zero-size blob — both are valid.
    const auto type = sel.columnOptionalBlob (0);
    if (type.has_value())
        EXPECT_EQ (0u, type->size());
}

//==============================================================================
// bindNull + optional columns
//==============================================================================

TEST_F (SqliteDatabaseTests, BindNullMakesColumnNull)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindNull (1);
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalInt (0).has_value());
}

TEST_F (SqliteDatabaseTests, ColumnOptionalBoolReturnsNulloptForNull)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (NULL)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalBool (0).has_value());
}

TEST_F (SqliteDatabaseTests, ColumnOptionalBoolReturnsValueForNonNull)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (1)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    ASSERT_TRUE (sel.columnOptionalBool (0).has_value());
    EXPECT_TRUE (*sel.columnOptionalBool (0));
}

TEST_F (SqliteDatabaseTests, ColumnOptionalIntNullopt)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (NULL)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalInt (0).has_value());
}

TEST_F (SqliteDatabaseTests, ColumnOptionalIntValue)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (42)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    ASSERT_TRUE (sel.columnOptionalInt (0).has_value());
    EXPECT_EQ (42, *sel.columnOptionalInt (0));
}

TEST_F (SqliteDatabaseTests, ColumnOptionalInt64)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    const int64_t big = 9000000000LL;
    db.executeQuery ("INSERT INTO t VALUES (" + String (big) + ")");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    ASSERT_TRUE (sel.columnOptionalInt64 (0).has_value());
    EXPECT_EQ (big, *sel.columnOptionalInt64 (0));
}

TEST_F (SqliteDatabaseTests, ColumnOptionalDoubleNullopt)
{
    db.executeQuery ("CREATE TABLE t (v REAL)");
    db.executeQuery ("INSERT INTO t VALUES (NULL)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalDouble (0).has_value());
}

TEST_F (SqliteDatabaseTests, ColumnOptionalDoubleValue)
{
    db.executeQuery ("CREATE TABLE t (v REAL)");
    db.executeQuery ("INSERT INTO t VALUES (2.718)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    ASSERT_TRUE (sel.columnOptionalDouble (0).has_value());
    EXPECT_DOUBLE_EQ (2.718, *sel.columnOptionalDouble (0));
}

TEST_F (SqliteDatabaseTests, ColumnOptionalTextNullopt)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    db.executeQuery ("INSERT INTO t VALUES (NULL)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalText (0).has_value());
}

TEST_F (SqliteDatabaseTests, ColumnOptionalTextValue)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    db.executeQuery ("INSERT INTO t VALUES ('yup')");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    ASSERT_TRUE (sel.columnOptionalText (0).has_value());
    EXPECT_EQ ("yup", *sel.columnOptionalText (0));
}

TEST_F (SqliteDatabaseTests, ColumnOptionalFileNullopt)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    db.executeQuery ("INSERT INTO t VALUES (NULL)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalFile (0).has_value());
}

TEST_F (SqliteDatabaseTests, ColumnOptionalFileValue)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");

    const File file = File::getSpecialLocation (File::tempDirectory).getChildFile ("foo.db");

    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        EXPECT_TRUE (ins.bindText (1, file.getFullPathName()).wasOk());
        EXPECT_EQ (SQLITE_DONE, ins.step());
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    ASSERT_TRUE (sel.columnOptionalFile (0).has_value());
    EXPECT_EQ (file.getFullPathName(), sel.columnOptionalFile (0)->getFullPathName());
}

TEST_F (SqliteDatabaseTests, ColumnOptionalBlobNullopt)
{
    db.executeQuery ("CREATE TABLE t (v BLOB)");
    db.executeQuery ("INSERT INTO t VALUES (NULL)");

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalBlob (0).has_value());
}

TEST_F (SqliteDatabaseTests, ColumnOptionalBlobValue)
{
    db.executeQuery ("CREATE TABLE t (v BLOB)");
    const std::vector<uint8> data = { 0x01, 0x02, 0x03 };
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        ins.bindBlob (1, Span<const uint8> (data.data(), data.size()));
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    auto result = sel.columnOptionalBlob (0);
    ASSERT_TRUE (result.has_value());
    ASSERT_EQ (3u, result->size());
    EXPECT_EQ (0x01, (*result)[0]);
    EXPECT_EQ (0x02, (*result)[1]);
    EXPECT_EQ (0x03, (*result)[2]);
}

//==============================================================================
// Bind on invalid statement returns false
//==============================================================================

TEST (SqliteDatabaseStatementInvalidTests, BindOnInvalidStatementReturnsFalse)
{
    SqliteDatabase db;
    auto stmt = db.prepareStatement ("SELECT 1"); // db not open
    EXPECT_FALSE (stmt.isValid());
    EXPECT_FALSE (stmt.bindNull (1));
    EXPECT_FALSE (stmt.bindBool (1, true));
    EXPECT_FALSE (stmt.bindInt (1, 0));
    EXPECT_FALSE (stmt.bindInt64 (1, 0));
    EXPECT_FALSE (stmt.bindDouble (1, 0.0));
    EXPECT_FALSE (stmt.bindText (1, "x"));
    EXPECT_FALSE (stmt.bindFile (1, File()));
    EXPECT_FALSE (stmt.bindBlob (1, Span<const uint8>()));
}

//==============================================================================
// Transaction — commit
//==============================================================================

TEST_F (SqliteDatabaseTests, TransactionCommitsOnDestruction)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");

    {
        auto tx = db.beginTransaction();
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (1)");
        ins.step();
    } // tx commits here

    auto sel = db.prepareStatement ("SELECT COUNT(*) FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (1, sel.columnInt (0));
}

TEST_F (SqliteDatabaseTests, TransactionRollbackPreventsPersistence)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");

    {
        auto tx = db.beginTransaction();
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (1)");
        ins.step();
        tx.rollback();
    } // tx rolls back here

    auto sel = db.prepareStatement ("SELECT COUNT(*) FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (0, sel.columnInt (0));
}

TEST_F (SqliteDatabaseTests, TransactionCanContainMultipleInserts)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");

    {
        auto tx = db.beginTransaction();
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (?)");
        for (int i = 1; i <= 100; ++i)
        {
            ins.bindInt (1, i);
            ins.step();
            ins.reset();
        }
    }

    auto sel = db.prepareStatement ("SELECT COUNT(*) FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (100, sel.columnInt (0));
}

//==============================================================================
// Transaction — move semantics
//==============================================================================

TEST_F (SqliteDatabaseTests, TransactionMoveConstructorTransfersOwnership)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");

    {
        auto tx1 = db.beginTransaction();
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (5)");
        ins.step();

        auto tx2 = std::move (tx1);
        // tx1.db is now null — its destructor is a no-op.
        // tx2 holds the connection and commits on destruction.
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (5, sel.columnInt (0));
}

TEST_F (SqliteDatabaseTests, MovedFromTransactionRollsBackViaNewOwner)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");

    {
        auto tx = db.beginTransaction();
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (99)");
        ins.step();

        auto tx2 = std::move (tx);
        tx2.rollback();
        // tx2 will rollback; tx (moved-from, null db) does nothing
    }

    auto sel = db.prepareStatement ("SELECT COUNT(*) FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (0, sel.columnInt (0));
}

//==============================================================================
// readBlob
//==============================================================================

TEST_F (SqliteDatabaseTests, ReadBlobReadsDataIntoBuffer)
{
    // SQLite incremental blob I/O requires the table to have an explicit rowid.
    db.executeQuery ("CREATE TABLE blobs (id INTEGER PRIMARY KEY, data BLOB)");

    const std::vector<uint8> original = { 0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x01, 0x02, 0x03 };
    {
        auto ins = db.prepareStatement ("INSERT INTO blobs (id, data) VALUES (1, ?)");
        ins.bindBlob (1, Span<const uint8> (original.data(), original.size()));
        ins.step();
    }

    auto stmt = db.prepareStatement ("SELECT 1"); // statement just needs a valid db handle
    std::vector<uint8> readBack;

    const bool ok = stmt.readBlob ("blobs", "data", 1, [&] (int numBytes) -> void*
    {
        readBack.resize (static_cast<size_t> (numBytes));
        return readBack.data();
    });

    EXPECT_TRUE (ok);
    ASSERT_EQ (original.size(), readBack.size());
    EXPECT_EQ (0, std::memcmp (original.data(), readBack.data(), original.size()));
}

TEST (SqliteDatabaseReadBlobTests, ReadBlobOnClosedDatabaseReturnsFalse)
{
    SqliteDatabase closed;
    auto stmt = closed.prepareStatement ("SELECT 1");
    bool called = false;
    EXPECT_FALSE (stmt.readBlob ("t", "data", 1, [&] (int) -> void*
    {
        called = true;
        return nullptr;
    }));
    EXPECT_FALSE (called);
}

TEST_F (SqliteDatabaseTests, ReadBlobWithWrongTableReturnsFalse)
{
    auto stmt = db.prepareStatement ("SELECT 1");
    EXPECT_FALSE (stmt.readBlob ("no_such_table", "no_such_col", 1, [] (int) -> void*
    {
        return nullptr;
    }));
}

//==============================================================================
// Parameterised statement reuse
//==============================================================================

TEST_F (SqliteDatabaseTests, ReusedStatementWithResetProducesCorrectResults)
{
    db.executeQuery ("CREATE TABLE nums (v INTEGER)");

    auto ins = db.prepareStatement ("INSERT INTO nums VALUES (?)");
    for (int i = 1; i <= 5; ++i)
    {
        EXPECT_TRUE (ins.isValid());
        EXPECT_TRUE (ins.bindInt (1, i * i));
        EXPECT_EQ (SQLITE_DONE, ins.step());
        ins.reset();
    }

    auto sel = db.prepareStatement ("SELECT v FROM nums ORDER BY v");
    int count = 0;
    while (sel.step() == SQLITE_ROW)
    {
        const int expected = (count + 1) * (count + 1);
        EXPECT_EQ (expected, sel.columnInt (0));
        ++count;
    }
    EXPECT_EQ (5, count);
}

//==============================================================================
// Mixed NULL and non-NULL rows
//==============================================================================

TEST_F (SqliteDatabaseTests, MixedNullAndNonNullRows)
{
    db.executeQuery ("CREATE TABLE t (a INTEGER, b TEXT)");
    db.executeQuery ("INSERT INTO t VALUES (1, 'one')");
    db.executeQuery ("INSERT INTO t VALUES (NULL, NULL)");
    db.executeQuery ("INSERT INTO t VALUES (3, 'three')");

    auto sel = db.prepareStatement ("SELECT a, b FROM t ORDER BY COALESCE(a, 2)");

    ASSERT_EQ (SQLITE_ROW, sel.step());
    ASSERT_TRUE (sel.columnOptionalInt (0).has_value());
    ASSERT_TRUE (sel.columnOptionalText (1).has_value());
    EXPECT_EQ (1, *sel.columnOptionalInt (0));
    EXPECT_EQ ("one", *sel.columnOptionalText (1));

    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalInt (0).has_value());
    EXPECT_FALSE (sel.columnOptionalText (1).has_value());

    ASSERT_EQ (SQLITE_ROW, sel.step());
    ASSERT_TRUE (sel.columnOptionalInt (0).has_value());
    ASSERT_TRUE (sel.columnOptionalText (1).has_value());
    EXPECT_EQ (3, *sel.columnOptionalInt (0));
    EXPECT_EQ ("three", *sel.columnOptionalText (1));
}

//==============================================================================
// executeQuery with named parameters works via prepareStatement
//==============================================================================

TEST_F (SqliteDatabaseTests, PrepareStatementWithNamedParameters)
{
    db.executeQuery ("CREATE TABLE t (id INTEGER, label TEXT)");
    {
        auto ins = db.prepareStatement ("INSERT INTO t VALUES (:id, :label)");
        EXPECT_TRUE (ins.isValid());

        // Named parameters are still bound by positional index (1-based).
        ins.bindInt (1, 7);
        ins.bindText (2, "seven");
        ins.step();
    }

    auto sel = db.prepareStatement ("SELECT id, label FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (7, sel.columnInt (0));
    EXPECT_EQ ("seven", sel.columnText (1));
}

//==============================================================================
// Result error messages
//==============================================================================

TEST (SqliteDatabaseResultTests, OpenReadOnlyMissingFileHasErrorMessage)
{
    SqliteDatabase db;
    File nonExistent (File::getSpecialLocation (File::tempDirectory)
                          .getChildFile ("yup_missing_" + String::toHexString (Random::getSystemRandom().nextInt()) + ".db"));

    auto result = db.open (nonExistent, true);
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.getErrorMessage().isNotEmpty());
}

TEST_F (SqliteDatabaseTests, ExecuteQueryBadSqlHasErrorMessage)
{
    auto result = db.executeQuery ("THIS IS NOT SQL");
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.getErrorMessage().isNotEmpty());
}

TEST_F (SqliteDatabaseTests, ExecuteQueryOnClosedDatabaseHasErrorMessage)
{
    SqliteDatabase closed;
    auto result = closed.executeQuery ("SELECT 1");
    EXPECT_FALSE (result.wasOk());
    EXPECT_EQ ("Database is not open", result.getErrorMessage());
}

TEST_F (SqliteDatabaseTests, BindOnInvalidStatementHasErrorMessage)
{
    SqliteDatabase::Statement invalid = db.prepareStatement ("BROKEN !!!");
    EXPECT_FALSE (invalid.isValid());

    auto r = invalid.bindInt (1, 0);
    EXPECT_FALSE (r.wasOk());
    EXPECT_EQ ("Statement is not valid", r.getErrorMessage());
}

TEST_F (SqliteDatabaseTests, BindColumnOutOfRangeHasErrorMessage)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    auto stmt = db.prepareStatement ("INSERT INTO t VALUES (?)");
    ASSERT_TRUE (stmt.isValid());

    // Column 99 doesn't exist — sqlite3 returns SQLITE_RANGE.
    auto r = stmt.bindInt (99, 0);
    EXPECT_FALSE (r.wasOk());
    EXPECT_TRUE (r.getErrorMessage().isNotEmpty());
}

TEST_F (SqliteDatabaseTests, ReadBlobOnInvalidTableHasErrorMessage)
{
    auto stmt = db.prepareStatement ("SELECT 1");
    auto result = stmt.readBlob ("no_such_table", "no_such_col", 1, [] (int) -> void*
    {
        return nullptr;
    });
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.getErrorMessage().isNotEmpty());
}

TEST (SqliteDatabaseResultTests, ReadBlobOnNullConnectionHasErrorMessage)
{
    SqliteDatabase closed;
    auto stmt = closed.prepareStatement ("SELECT 1");
    auto result = stmt.readBlob ("t", "data", 1, [] (int) -> void*
    {
        return nullptr;
    });
    EXPECT_FALSE (result.wasOk());
    EXPECT_EQ ("Statement has no database connection", result.getErrorMessage());
}

//==============================================================================
// Variadic prepareStatement — success paths
//==============================================================================

TEST_F (SqliteDatabaseTests, PrepareWithSingleIntArg)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (42)");

    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", 42);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
    EXPECT_EQ (42, result.getReference().columnInt (0));
}

TEST_F (SqliteDatabaseTests, PrepareWithSingleInt64Arg)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    const int64_t big = 9'000'000'000LL;
    db.executeQuery ("INSERT INTO t VALUES (" + String (big) + ")");

    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", big);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
    EXPECT_EQ (big, result.getReference().columnInt64 (0));
}

TEST_F (SqliteDatabaseTests, PrepareWithSingleDoubleArg)
{
    db.executeQuery ("CREATE TABLE t (v REAL)");
    db.executeQuery ("INSERT INTO t VALUES (2.718)");

    auto result = db.prepareStatement ("SELECT v FROM t WHERE v > ?", 2.0);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
    EXPECT_DOUBLE_EQ (2.718, result.getReference().columnDouble (0));
}

TEST_F (SqliteDatabaseTests, PrepareWithSingleFloatArgPromotedToDouble)
{
    db.executeQuery ("CREATE TABLE t (v REAL)");
    db.executeQuery ("INSERT INTO t VALUES (1.0)");

    auto result = db.prepareStatement ("SELECT v FROM t WHERE v < ?", 2.0f);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
}

TEST_F (SqliteDatabaseTests, PrepareWithSingleBoolArg)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (1)");

    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", true);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
    EXPECT_TRUE (result.getReference().columnBool (0));
}

TEST_F (SqliteDatabaseTests, PrepareWithStringLiteralArg)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    db.executeQuery ("INSERT INTO t VALUES ('hello')");

    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", "hello");
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
    EXPECT_EQ ("hello", result.getReference().columnText (0));
}

TEST_F (SqliteDatabaseTests, PrepareWithStringArg)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    db.executeQuery ("INSERT INTO t VALUES ('world')");

    const String text = "world";
    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", text);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
    EXPECT_EQ ("world", result.getReference().columnText (0));
}

TEST_F (SqliteDatabaseTests, PrepareWithFileArg)
{
    db.executeQuery ("CREATE TABLE t (v TEXT)");
    const File f ("/tmp/test.db");
    db.executeQuery ("INSERT INTO t VALUES ('" + f.getFullPathName() + "')");

    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", f);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
    EXPECT_EQ (f.getFullPathName(), result.getReference().columnFile (0).getFullPathName());
}

TEST_F (SqliteDatabaseTests, PrepareWithNullptrArgBindsNull)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    {
        auto result = db.prepareStatement ("INSERT INTO t VALUES (?)", nullptr);
        ASSERT_TRUE (result.wasOk());
        result.getReference().step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalInt (0).has_value());
}

TEST_F (SqliteDatabaseTests, PrepareWithNulloptArgBindsNull)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    {
        auto result = db.prepareStatement ("INSERT INTO t VALUES (?)", std::nullopt);
        ASSERT_TRUE (result.wasOk());
        result.getReference().step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_FALSE (sel.columnOptionalInt (0).has_value());
}

TEST_F (SqliteDatabaseTests, PrepareWithBlobArg)
{
    db.executeQuery ("CREATE TABLE t (v BLOB)");
    const std::vector<uint8> data = { 0xDE, 0xAD, 0xBE, 0xEF };
    {
        auto result = db.prepareStatement ("INSERT INTO t VALUES (?)",
                                           Span<const uint8> (data.data(), data.size()));
        ASSERT_TRUE (result.wasOk());
        result.getReference().step();
    }

    auto sel = db.prepareStatement ("SELECT v FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    const auto blob = sel.columnBlob (0);
    ASSERT_EQ (4u, blob.size());
    EXPECT_EQ (0, std::memcmp (data.data(), blob.data(), 4));
}

TEST_F (SqliteDatabaseTests, PrepareWithSmallIntegralTypesWidenedToInt)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (7)");

    const int16_t small = 7;
    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", small);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
}

TEST_F (SqliteDatabaseTests, PrepareWithUnsignedInt)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (100)");

    const unsigned int u = 100u;
    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", u);
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_ROW, result.getReference().step());
}

TEST_F (SqliteDatabaseTests, PrepareWithMultipleArgs)
{
    db.executeQuery ("CREATE TABLE t (id INTEGER, name TEXT, score REAL)");
    {
        auto result = db.prepareStatement ("INSERT INTO t VALUES (?, ?, ?)", 1, "alice", 9.5);
        ASSERT_TRUE (result.wasOk());
        EXPECT_EQ (SQLITE_DONE, result.getReference().step());
    }

    auto sel = db.prepareStatement ("SELECT id, name, score FROM t WHERE id = ?", 1);
    ASSERT_TRUE (sel.wasOk());
    ASSERT_EQ (SQLITE_ROW, sel.getReference().step());
    EXPECT_EQ (1, sel.getReference().columnInt (0));
    EXPECT_EQ ("alice", sel.getReference().columnText (1));
    EXPECT_DOUBLE_EQ (9.5, sel.getReference().columnDouble (2));
}

TEST_F (SqliteDatabaseTests, PrepareWithMixedTypesInsertAndQuery)
{
    db.executeQuery ("CREATE TABLE kv (key TEXT, ival INTEGER, rval REAL, bval INTEGER)");
    {
        auto result = db.prepareStatement ("INSERT INTO kv VALUES (?, ?, ?, ?)",
                                           "row1",
                                           int64_t { 999 },
                                           1.23,
                                           true);
        ASSERT_TRUE (result.wasOk());
        EXPECT_EQ (SQLITE_DONE, result.getReference().step());
    }

    auto sel = db.prepareStatement ("SELECT key, ival, rval, bval FROM kv WHERE key = ?", "row1");
    ASSERT_TRUE (sel.wasOk());
    ASSERT_EQ (SQLITE_ROW, sel.getReference().step());
    EXPECT_EQ ("row1", sel.getReference().columnText (0));
    EXPECT_EQ (999, sel.getReference().columnInt64 (1));
    EXPECT_DOUBLE_EQ (1.23, sel.getReference().columnDouble (2));
    EXPECT_TRUE (sel.getReference().columnBool (3));
}

TEST_F (SqliteDatabaseTests, PrepareVariadicInsideTransaction)
{
    db.executeQuery ("CREATE TABLE t (id INTEGER, val TEXT)");

    {
        auto tx = db.beginTransaction();
        for (int i = 1; i <= 3; ++i)
        {
            auto result = db.prepareStatement ("INSERT INTO t VALUES (?, ?)", i, "v" + String (i));
            ASSERT_TRUE (result.wasOk());
            EXPECT_EQ (SQLITE_DONE, result.getReference().step());
        }
    } // commits

    auto sel = db.prepareStatement ("SELECT COUNT(*) FROM t");
    ASSERT_EQ (SQLITE_ROW, sel.step());
    EXPECT_EQ (3, sel.columnInt (0));
}

TEST_F (SqliteDatabaseTests, PrepareVariadicResultCanBeMovedOut)
{
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (77)");

    auto result = db.prepareStatement ("SELECT v FROM t WHERE v = ?", 77);
    ASSERT_TRUE (result.wasOk());

    SqliteDatabase::Statement stmt = std::move (result).getValue();
    EXPECT_TRUE (stmt.isValid());
    EXPECT_EQ (SQLITE_ROW, stmt.step());
    EXPECT_EQ (77, stmt.columnInt (0));
}

//==============================================================================
// Variadic prepareStatement — failure paths
//==============================================================================

TEST_F (SqliteDatabaseTests, PrepareVariadicBadSqlFails)
{
    auto result = db.prepareStatement ("NOT VALID SQL !!!", 1, 2, 3);
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.getErrorMessage().isNotEmpty());
}

TEST_F (SqliteDatabaseTests, PrepareVariadicTooManyArgsFails)
{
    // SQL has 1 placeholder but we supply 3 args — caught by the parameter-count check.
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    auto result = db.prepareStatement ("INSERT INTO t VALUES (?)", 1, 2, 3);
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.getErrorMessage().containsIgnoreCase ("Expected 1"));
}

TEST_F (SqliteDatabaseTests, PrepareVariadicTooFewArgsFails)
{
    // SQL has 3 placeholders but we supply only 1 arg — caught by the parameter-count check.
    db.executeQuery ("CREATE TABLE t (a INTEGER, b INTEGER, c INTEGER)");
    auto result = db.prepareStatement ("INSERT INTO t VALUES (?, ?, ?)", 42);
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.getErrorMessage().containsIgnoreCase ("Expected 3"));
}

TEST_F (SqliteDatabaseTests, PrepareVariadicExactArgCountSucceeds)
{
    db.executeQuery ("CREATE TABLE t (a INTEGER, b TEXT)");
    auto result = db.prepareStatement ("INSERT INTO t VALUES (?, ?)", 1, "one");
    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (SQLITE_DONE, result.getReference().step());
}

TEST (SqliteDatabaseVariadicTests, PrepareVariadicOnClosedDatabaseFails)
{
    SqliteDatabase closed;
    auto result = closed.prepareStatement ("SELECT 1", 42);
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.getErrorMessage().isNotEmpty());
}

//==============================================================================
// vacuumToFile
//==============================================================================

TEST (SqliteDatabaseVacuumTests, VacuumToFileOnClosedDatabaseFails)
{
    SqliteDatabase closed;
    File dest = File::getSpecialLocation (File::tempDirectory)
                    .getChildFile ("yup_vacuum_closed_" + String::toHexString (Random::getSystemRandom().nextInt()) + ".db");

    EXPECT_FALSE (closed.vacuumToFile (dest));
    dest.deleteFile();
}

TEST (SqliteDatabaseVacuumTests, VacuumInMemoryDatabaseToFile)
{
    SqliteDatabase db;
    EXPECT_TRUE (db.openInMemory());

    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (1)");
    db.executeQuery ("INSERT INTO t VALUES (2)");

    File dest = File::getSpecialLocation (File::tempDirectory)
                    .getChildFile ("yup_vacuum_" + String::toHexString (Random::getSystemRandom().nextInt()) + ".db");

    EXPECT_TRUE (db.vacuumToFile (dest));
    EXPECT_TRUE (dest.existsAsFile());

    // Reopen the vacuumed file and verify the data is intact.
    {
        SqliteDatabase copy;
        EXPECT_TRUE (copy.open (dest));

        std::vector<int> values;
        copy.executeQuery ("SELECT v FROM t ORDER BY v",
                           [&] (int, char** argv, char**)
        {
            values.push_back (std::stoi (argv[0]));
            return true;
        });

        ASSERT_EQ (2u, values.size());
        EXPECT_EQ (1, values[0]);
        EXPECT_EQ (2, values[1]);
    }

    dest.deleteFile();
}

TEST (SqliteDatabaseVacuumTests, VacuumOverwritesExistingFile)
{
    SqliteDatabase db;
    EXPECT_TRUE (db.openInMemory());
    db.executeQuery ("CREATE TABLE t (v INTEGER)");
    db.executeQuery ("INSERT INTO t VALUES (42)");

    File dest = File::getSpecialLocation (File::tempDirectory)
                    .getChildFile ("yup_vacuum_overwrite_" + String::toHexString (Random::getSystemRandom().nextInt()) + ".db");

    // Create the destination file first so we exercise the overwrite path.
    EXPECT_TRUE (db.vacuumToFile (dest));
    EXPECT_TRUE (dest.existsAsFile());

    db.executeQuery ("INSERT INTO t VALUES (99)");
    EXPECT_TRUE (db.vacuumToFile (dest));

    SqliteDatabase copy;
    EXPECT_TRUE (copy.open (dest));

    std::vector<int> values;
    copy.executeQuery ("SELECT v FROM t ORDER BY v",
                       [&] (int, char** argv, char**)
    {
        values.push_back (std::stoi (argv[0]));
        return true;
    });

    ASSERT_EQ (2u, values.size());
    EXPECT_EQ (42, values[0]);
    EXPECT_EQ (99, values[1]);

    dest.deleteFile();
}

#endif // YUP_MODULE_AVAILABLE_sqlite3_library
