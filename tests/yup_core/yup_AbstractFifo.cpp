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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

namespace
{

struct WriteThread : public Thread
{
    WriteThread (AbstractFifo& fifo, int* buffer)
        : Thread ("fifo writer")
        , fifo (fifo)
        , buffer (buffer)
    {
        startThread();
    }

    ~WriteThread() override
    {
        stopThread (5000);
    }

    void run() override
    {
        int n = 0;
        auto& random = Random::getSystemRandom();

        while (! threadShouldExit())
        {
            int num = random.nextInt (2000) + 1;

            auto writer = fifo.write (num);

            ASSERT_TRUE (writer.blockSize1 >= 0 && writer.blockSize2 >= 0);
            ASSERT_TRUE (writer.blockSize1 == 0
                         || (writer.startIndex1 >= 0 && writer.startIndex1 < fifo.getTotalSize()));
            ASSERT_TRUE (writer.blockSize2 == 0
                         || (writer.startIndex2 >= 0 && writer.startIndex2 < fifo.getTotalSize()));

            writer.forEach ([this, &n] (int index)
            {
                this->buffer[index] = n++;
            });
        }
    }

    AbstractFifo& fifo;
    int* buffer;
};

} // namespace

TEST (AbstractFifoTests, BasicFunctionality)
{
    int buffer[5000];
    AbstractFifo fifo (numElementsInArray (buffer));

    WriteThread writer (fifo, buffer);

    int n = 0;
    Random r;
    r.combineSeed (12345);

    for (int count = 100000; --count >= 0;)
    {
        int num = r.nextInt (6000) + 1;

        auto reader = fifo.read (num);

        ASSERT_TRUE (reader.blockSize1 >= 0 && reader.blockSize2 >= 0);
        ASSERT_TRUE (reader.blockSize1 == 0
                     || (reader.startIndex1 >= 0 && reader.startIndex1 < fifo.getTotalSize()));
        ASSERT_TRUE (reader.blockSize2 == 0
                     || (reader.startIndex2 >= 0 && reader.startIndex2 < fifo.getTotalSize()));

        bool failed = false;

        reader.forEach ([&failed, &buffer, &n] (int index)
        {
            failed = (buffer[index] != n++) || failed;
        });

        ASSERT_FALSE (failed) << "Read values were incorrect";
    }
}

TEST (AbstractFifoTests, Constructor)
{
    AbstractFifo fifo (10);
    EXPECT_EQ (fifo.getTotalSize(), 10);
    EXPECT_EQ (fifo.getFreeSpace(), 9);
    EXPECT_EQ (fifo.getNumReady(), 0);
}

TEST (AbstractFifoTests, Reset)
{
    AbstractFifo fifo (10);
    int startIndex1, blockSize1, startIndex2, blockSize2;

    fifo.prepareToWrite (5, startIndex1, blockSize1, startIndex2, blockSize2);
    fifo.finishedWrite (5);

    fifo.reset();
    EXPECT_EQ (fifo.getFreeSpace(), 9);
    EXPECT_EQ (fifo.getNumReady(), 0);
}

TEST (AbstractFifoTests, SetTotalSize)
{
    AbstractFifo fifo (10);
    fifo.setTotalSize (20);
    EXPECT_EQ (fifo.getTotalSize(), 20);
    EXPECT_EQ (fifo.getFreeSpace(), 19);
    EXPECT_EQ (fifo.getNumReady(), 0);
}

TEST (AbstractFifoTests, PrepareToWrite)
{
    AbstractFifo fifo (11);
    int startIndex1, blockSize1, startIndex2, blockSize2;

    fifo.prepareToWrite (5, startIndex1, blockSize1, startIndex2, blockSize2);
    EXPECT_EQ (blockSize1, 5);
    EXPECT_EQ (blockSize2, 0);
    fifo.finishedWrite (5);

    fifo.prepareToWrite (10, startIndex1, blockSize1, startIndex2, blockSize2);
    EXPECT_EQ (blockSize1, 5);
    EXPECT_EQ (blockSize2, 0);
}

TEST (AbstractFifoTests, PrepareToRead)
{
    AbstractFifo fifo (10);
    int startIndex1, blockSize1, startIndex2, blockSize2;

    fifo.prepareToWrite (5, startIndex1, blockSize1, startIndex2, blockSize2);
    fifo.finishedWrite (5);

    fifo.prepareToRead (5, startIndex1, blockSize1, startIndex2, blockSize2);
    EXPECT_EQ (blockSize1, 5);
    EXPECT_EQ (blockSize2, 0);

    fifo.finishedRead (5);
    fifo.prepareToRead (5, startIndex1, blockSize1, startIndex2, blockSize2);
    EXPECT_EQ (blockSize1, 0);
    EXPECT_EQ (blockSize2, 0);
}

TEST (AbstractFifoTests, WriteReadCycle)
{
    AbstractFifo fifo (11);
    int startIndex1, blockSize1, startIndex2, blockSize2;

    // Write first half
    fifo.prepareToWrite (5, startIndex1, blockSize1, startIndex2, blockSize2);
    fifo.finishedWrite (5);

    // Write second half
    fifo.prepareToWrite (5, startIndex1, blockSize1, startIndex2, blockSize2);
    fifo.finishedWrite (5);

    EXPECT_EQ (fifo.getNumReady(), 10);
    EXPECT_EQ (fifo.getFreeSpace(), 0);

    // Read first half
    fifo.prepareToRead (5, startIndex1, blockSize1, startIndex2, blockSize2);
    EXPECT_EQ (blockSize1, 5);
    EXPECT_EQ (blockSize2, 0);
    fifo.finishedRead (5);

    // Read second half
    fifo.prepareToRead (5, startIndex1, blockSize1, startIndex2, blockSize2);
    EXPECT_EQ (blockSize1, 5);
    EXPECT_EQ (blockSize2, 0);
    fifo.finishedRead (5);

    EXPECT_EQ (fifo.getNumReady(), 0);
    EXPECT_EQ (fifo.getFreeSpace(), 10);
}

TEST (AbstractFifoTests, WriteWrapAround)
{
    AbstractFifo fifo (10);
    int startIndex1, blockSize1, startIndex2, blockSize2;

    // Write to nearly full
    fifo.prepareToWrite (9, startIndex1, blockSize1, startIndex2, blockSize2);
    fifo.finishedWrite (9);

    // Read some to make space at the beginning
    fifo.prepareToRead (5, startIndex1, blockSize1, startIndex2, blockSize2);
    fifo.finishedRead (5);

    // Write more, causing wrap around
    fifo.prepareToWrite (5, startIndex1, blockSize1, startIndex2, blockSize2);
    EXPECT_EQ (blockSize1, 1);
    EXPECT_EQ (blockSize2, 4);
    fifo.finishedWrite (5);

    // Read all, checking wrap around handling
    fifo.prepareToRead (10, startIndex1, blockSize1, startIndex2, blockSize2);
    EXPECT_EQ (blockSize1, 5);
    EXPECT_EQ (blockSize2, 4);
    fifo.finishedRead (9);

    EXPECT_EQ (fifo.getNumReady(), 0);
    EXPECT_EQ (fifo.getFreeSpace(), 9);
}

TEST (AbstractFifoTests, ScopedWriteRead)
{
    AbstractFifo fifo (10);

    {
        auto writeHandle = fifo.write (7);
        EXPECT_EQ (writeHandle.blockSize1, 7);
        EXPECT_EQ (writeHandle.blockSize2, 0);
    } // writeHandle goes out of scope here

    EXPECT_EQ (fifo.getNumReady(), 7);
    EXPECT_EQ (fifo.getFreeSpace(), 2);

    {
        auto readHandle = fifo.read (5);
        EXPECT_EQ (readHandle.blockSize1, 5);
        EXPECT_EQ (readHandle.blockSize2, 0);
    } // readHandle goes out of scope here

    EXPECT_EQ (fifo.getNumReady(), 2);
    EXPECT_EQ (fifo.getFreeSpace(), 7);
}

TEST (AbstractFifoTests, ScopedWriteReadWrapAround)
{
    AbstractFifo fifo (10);

    {
        auto writeHandle = fifo.write (9);
        EXPECT_EQ (writeHandle.blockSize1, 9);
        EXPECT_EQ (writeHandle.blockSize2, 0);
    } // writeHandle goes out of scope here

    {
        auto readHandle = fifo.read (5);
        EXPECT_EQ (readHandle.blockSize1, 5);
        EXPECT_EQ (readHandle.blockSize2, 0);
    } // readHandle goes out of scope here

    {
        auto writeHandle = fifo.write (5);
        EXPECT_EQ (writeHandle.blockSize1, 1);
        EXPECT_EQ (writeHandle.blockSize2, 4);
    } // writeHandle goes out of scope here

    {
        auto readHandle = fifo.read (10);
        EXPECT_EQ (readHandle.blockSize1, 5);
        EXPECT_EQ (readHandle.blockSize2, 4);
    } // readHandle goes out of scope here

    EXPECT_EQ (fifo.getNumReady(), 0);
    EXPECT_EQ (fifo.getFreeSpace(), 9);
}

TEST (AbstractFifoTests, AbstractFifoThreaded)
{
    struct WriteThread : public Thread
    {
        WriteThread (AbstractFifo& f, int* b, Random rng)
            : Thread ("WriterThread")
            , fifo (f)
            , buffer (b)
            , random (rng)
        {
        }

        ~WriteThread()
        {
            signalThreadShouldExit();
            stopThread (-1);
        }

        void run()
        {
            int n = 0;
            while (! threadShouldExit())
            {
                int num = random.nextInt (2000) + 1;
                auto writer = fifo.write (num);

                ASSERT_GE (writer.blockSize1, 0);
                ASSERT_GE (writer.blockSize2, 0);

                ASSERT_TRUE (writer.blockSize1 == 0
                             || (writer.startIndex1 >= 0 && writer.startIndex1 < fifo.getTotalSize()));

                ASSERT_TRUE (writer.blockSize2 == 0
                             || (writer.startIndex2 >= 0 && writer.startIndex2 < fifo.getTotalSize()));

                writer.forEach ([this, &n] (int index)
                {
                    this->buffer[index] = n++;
                });
            }
        }

        AbstractFifo& fifo;
        int* buffer;
        Random random;
    };

    int buffer[5000];
    AbstractFifo fifo (sizeof (buffer) / sizeof (buffer[0]));

    Random random;
    WriteThread writer (fifo, buffer, random);

    int n = 0;
    Random r;
    r.combineSeed (12345);

    for (int count = 100000; --count >= 0;)
    {
        int num = r.nextInt (6000) + 1;
        auto reader = fifo.read (num);

        if (! (reader.blockSize1 >= 0 && reader.blockSize2 >= 0)
            && (reader.blockSize1 == 0
                || (reader.startIndex1 >= 0 && reader.startIndex1 < fifo.getTotalSize()))
            && (reader.blockSize2 == 0
                || (reader.startIndex2 >= 0 && reader.startIndex2 < fifo.getTotalSize())))
        {
            FAIL() << "prepareToRead returned negative values";
            break;
        }

        bool failed = false;

        reader.forEach ([&failed, &buffer, &n] (int index)
        {
            failed = (buffer[index] != n++) || failed;
        });

        if (failed)
        {
            FAIL() << "read values were incorrect";
            break;
        }
    }
}
