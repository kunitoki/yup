/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <juce_core/juce_core.h>

#include <unordered_map>

using namespace yup;

TEST (IdentifierTests, DefaultConstructorCreatesNullIdentifier)
{
    Identifier id;
    EXPECT_TRUE (id.isNull());
    EXPECT_FALSE (id.isValid());
}

TEST (IdentifierTests, ConstructFromStringLiteral)
{
    Identifier id ("test");
    EXPECT_EQ (id.toString(), "test");
    EXPECT_TRUE (id.isValid());
}

TEST (IdentifierTests, ConstructFromStringObject)
{
    String name = "example";
    Identifier id (name);
    EXPECT_EQ (id.toString(), "example");
    EXPECT_TRUE (id.isValid());
}

TEST (IdentifierTests, CopyConstructor)
{
    Identifier original ("copyTest");
    Identifier copy = original;
    EXPECT_EQ (copy, original);
}

TEST (IdentifierTests, MoveConstructor)
{
    Identifier original ("moveTest");
    Identifier moved = std::move (original);
    EXPECT_EQ (moved.toString(), "moveTest");
}

TEST (IdentifierTests, AssignmentOperator)
{
    Identifier id1 ("first");
    Identifier id2 = id1;
    EXPECT_EQ (id2, id1);
}

TEST (IdentifierTests, MoveAssignmentOperator)
{
    Identifier id1 ("first");
    Identifier id2 ("second");
    id2 = std::move (id1);
    EXPECT_EQ (id2.toString(), "first");
}

TEST (IdentifierTests, ComparisonOperators)
{
    Identifier id1 ("same");
    Identifier id2 ("same");
    Identifier id3 ("different");

    EXPECT_TRUE (id1 == id2);
    EXPECT_FALSE (id1 == id3);
    EXPECT_TRUE (id1 != id3);
    EXPECT_FALSE (id1 != id2);
}

TEST (IdentifierTests, IsValidIdentifier)
{
    EXPECT_TRUE (Identifier::isValidIdentifier ("valid_name"));
    EXPECT_FALSE (Identifier::isValidIdentifier ("invalid name"));
    EXPECT_TRUE (Identifier::isValidIdentifier ("123"));
    EXPECT_TRUE (Identifier::isValidIdentifier ("_123"));
    EXPECT_FALSE (Identifier::isValidIdentifier ("_1 23"));
}

TEST (IdentifierTests, ConversionToStringRef)
{
    Identifier id ("conversion");
    StringRef ref = id;
    EXPECT_EQ (ref, StringRef ("conversion"));
}

TEST (IdentifierTests, ConversionToCharPointer)
{
    Identifier id ("pointer");
    auto ptr = id.getCharPointer();
    EXPECT_STREQ (ptr.getAddress(), "pointer");
}

TEST (Identifier, UseInAssociativeContainers)
{
    std::unordered_map<Identifier, Identifier> ids;
    ids[Identifier ("test1")] = Identifier ("test2");

    ASSERT_TRUE (ids.find (Identifier ("test1")) != ids.end());
    EXPECT_EQ (ids.find (Identifier ("test1"))->first, Identifier ("test1"));
    EXPECT_EQ (ids.find (Identifier ("test1"))->second, Identifier ("test2"));
}
