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

#include <yup_data_model/yup_data_model.h>

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

using namespace yup;

//==============================================================================
/** Measures DataTree edits at two sizes: a cost that grows faster than the data shows
    as a higher time per operation at the larger size.

    Disabled by default, run it in Release with:
    GTEST_ALSO_RUN_DISABLED_TESTS=1 just test Release "*DataTreeBenchmark*" yup_data_model
*/
class DataTreeBenchmarkTests : public ::testing::Test
{
protected:
    static constexpr int numRuns = 3;
    static constexpr int sizes[] = { 2000, 20000 };
    static constexpr int fanOut = 4;

    //==============================================================================
    /** Returns the best time of `run (state)` in seconds. Every run gets a fresh
        `prepare()`, which is not timed, nor is the destruction of what it returned.
    */
    template <class Prepare, class Run>
    static double measureSeconds (Prepare&& prepare, Run&& run)
    {
        auto best = std::numeric_limits<double>::max();

        for (int i = 0; i < numRuns; ++i)
        {
            auto state = prepare();

            const auto start = std::chrono::steady_clock::now();
            run (state);
            best = jmin (best, std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count());
        }

        return best;
    }

    static void printHeader (const char* what)
    {
        std::cout << "\n  |==== BENCH ==== (DataTree " << what << ", best of " << numRuns << ")\n";
    }

    static void report (const String& name, int size, double seconds, int numOperations)
    {
        std::cout << "  " << name.paddedRight (' ', 48)
                  << String (size).paddedLeft (' ', 7)
                  << String (seconds * 1.0e9 / numOperations, 1).paddedLeft (' ', 16) << " ns/op"
                  << String (seconds * 1.0e3, 2).paddedLeft (' ', 12) << " ms\n";
    }

    //==============================================================================
    struct Family
    {
        DataTree parent = DataTree ("Parent");
        std::vector<DataTree> children;
        UndoManager::Ptr undoManager;
    };

    static Family makeFamily (int numChildren, bool attached, bool undoable = false)
    {
        Family family;
        family.children.reserve (static_cast<size_t> (numChildren));

        for (int i = 0; i < numChildren; ++i)
            family.children.emplace_back ("Child");

        // Attached in small transactions, so that preparing stays cheap
        constexpr size_t chunkSize = 64;

        for (size_t start = 0; attached && start < family.children.size(); start += chunkSize)
        {
            auto transaction = family.parent.beginTransaction();

            for (auto i = start; i < jmin (start + chunkSize, family.children.size()); ++i)
                transaction.addChild (family.children[i]);
        }

        if (undoable)
            family.undoManager = new UndoManager (RelativeTime());

        return family;
    }

    //==============================================================================
    static DataTree buildBottomUp (int depth)
    {
        DataTree node ("Node");

        if (depth > 0)
        {
            std::vector<DataTree> children;

            for (int i = 0; i < fanOut; ++i)
                children.push_back (buildBottomUp (depth - 1));

            auto transaction = node.beginTransaction();

            for (const auto& child : children)
                transaction.addChild (child);
        }

        return node;
    }

    static void fillTopDown (DataTree node, int depth)
    {
        if (depth == 0)
            return;

        std::vector<DataTree> children;

        {
            auto transaction = node.beginTransaction();

            for (int i = 0; i < fanOut; ++i)
                transaction.addChild (children.emplace_back ("Node"));
        }

        for (const auto& child : children)
            fillTopDown (child, depth - 1);
    }

    static int countNodes (int depth)
    {
        int total = 0;

        for (int level = 0, width = 1; level <= depth; ++level, width *= fanOut)
            total += width;

        return total;
    }

    //==============================================================================
    struct Item
    {
        explicit Item (const DataTree& treeToUse)
            : tree (treeToUse)
        {
        }

        DataTree getDataTree() const { return tree; }

        DataTree tree;
    };

    struct ItemList final : DataTreeObjectList<Item>
    {
        explicit ItemList (const DataTree& parentTree)
            : DataTreeObjectList<Item> (parentTree)
        {
            rebuildObjects();
        }

        ~ItemList() override
        {
            freeObjects();
        }

        bool isSuitableType (const DataTree&) const override { return true; }

        Item* createNewObject (const DataTree& tree) override { return new Item (tree); }

        void deleteObject (Item* item) override { delete item; }
    };

    struct Listed
    {
        Family family;
        std::unique_ptr<ItemList> list;
    };

    static Listed makeListed (int numChildren, bool attached)
    {
        Listed listed { makeFamily (numChildren, attached), nullptr };
        listed.list = std::make_unique<ItemList> (listed.family.parent);
        return listed;
    }
};

//==============================================================================
TEST_F (DataTreeBenchmarkTests, DISABLED_Properties)
{
    printHeader ("properties");

    const Identifier value ("value");

    for (const auto size : sizes)
    {
        std::vector<Identifier> names;

        for (int i = 0; i < size; ++i)
            names.emplace_back ("p" + String (i));

        const auto makeNode = []
        {
            return DataTree ("Node");
        };

        report ("set N properties, 1 transaction", size, measureSeconds (makeNode, [&] (DataTree& node)
        {
            {
                auto transaction = node.beginTransaction();

                for (int i = 0; i < size; ++i)
                    transaction.setProperty (names[static_cast<size_t> (i)], i);
            }

            EXPECT_EQ (size, node.getNumProperties());
        }),
                size);

        report ("set 1 property, N transactions", size, measureSeconds (makeNode, [&] (DataTree& node)
        {
            for (int i = 0; i < size; ++i)
            {
                auto transaction = node.beginTransaction();
                transaction.setProperty (value, i);
            }

            EXPECT_EQ (var (size - 1), node.getProperty (value));
        }),
                size);
    }
}

TEST_F (DataTreeBenchmarkTests, DISABLED_Children)
{
    printHeader ("children");

    for (const auto size : sizes)
    {
        const auto detached = [size]
        {
            return makeFamily (size, false);
        };

        const auto detachedUndoable = [size]
        {
            return makeFamily (size, false, true);
        };

        const auto attached = [size]
        {
            return makeFamily (size, true);
        };

        report ("append N children", size, measureSeconds (detached, [size] (Family& family)
        {
            {
                auto transaction = family.parent.beginTransaction();

                for (const auto& child : family.children)
                    transaction.addChild (child);
            }

            EXPECT_EQ (size, family.parent.getNumChildren());
        }),
                size);

        report ("append N children, undoable", size, measureSeconds (detachedUndoable, [size] (Family& family)
        {
            {
                auto transaction = family.parent.beginTransaction (family.undoManager.get());

                for (const auto& child : family.children)
                    transaction.addChild (child);
            }

            EXPECT_EQ (size, family.parent.getNumChildren());
        }),
                size);

        report ("insert N children at the front", size, measureSeconds (detached, [size] (Family& family)
        {
            {
                auto transaction = family.parent.beginTransaction();

                for (const auto& child : family.children)
                    transaction.addChild (child, 0);
            }

            EXPECT_EQ (size, family.parent.getNumChildren());
        }),
                size);

        report ("insert N children at the front, together", size, measureSeconds (detached, [size] (Family& family)
        {
            {
                auto transaction = family.parent.beginTransaction();
                transaction.addChildren (family.children, 0);
            }

            EXPECT_EQ (size, family.parent.getNumChildren());
        }),
                size);

        report ("remove every other child of N, together", size, measureSeconds (attached, [size] (Family& family)
        {
            std::vector<DataTree> listed;

            for (size_t i = 0; i < family.children.size(); i += 2)
                listed.push_back (family.children[i]);

            {
                auto transaction = family.parent.beginTransaction();
                transaction.removeChildren (listed);
            }

            EXPECT_EQ (size / 2, family.parent.getNumChildren());
        }),
                size / 2);

        report ("remove every other child of N", size, measureSeconds (attached, [size] (Family& family)
        {
            {
                auto transaction = family.parent.beginTransaction();

                for (size_t i = 0; i < family.children.size(); i += 2)
                    transaction.removeChild (family.children[i]);
            }

            EXPECT_EQ (size / 2, family.parent.getNumChildren());
        }),
                size / 2);

        report ("reverse N children, together", size, measureSeconds (attached, [size] (Family& family)
        {
            const std::vector<DataTree> reversed (family.children.rbegin(), family.children.rend());

            {
                auto transaction = family.parent.beginTransaction();
                transaction.reorderChildren (reversed);
            }

            EXPECT_EQ (family.children.back(), family.parent.getChild (0));
        }),
                size);

        report ("move N/10 children of N", size, measureSeconds (attached, [size] (Family& family)
        {
            Random random (1);

            {
                auto transaction = family.parent.beginTransaction();

                for (int i = 0; i < size / 10; ++i)
                    transaction.moveChild (random.nextInt (size), random.nextInt (size));
            }

            EXPECT_EQ (size, family.parent.getNumChildren());
        }),
                size / 10);
    }
}

TEST_F (DataTreeBenchmarkTests, DISABLED_ManyNodes)
{
    printHeader ("many nodes");

    const Identifier selected ("selected");

    for (const auto size : sizes)
    {
        const auto plain = [size]
        {
            return makeFamily (size, true);
        };

        const auto undoable = [size]
        {
            return makeFamily (size, true, true);
        };

        const auto selectAll = [&selected] (Family& family)
        {
            for (auto& child : family.children)
            {
                auto transaction = child.beginTransaction (family.undoManager.get());
                transaction.setProperty (selected, true);
            }

            EXPECT_TRUE (family.children.back().hasProperty (selected));
        };

        const auto selectAllTogether = [&selected] (Family& family)
        {
            {
                DataTree::BatchTransaction batch (family.undoManager.get());

                for (const auto& child : family.children)
                    batch.edit (child).setProperty (selected, true);
            }

            EXPECT_TRUE (family.children.back().hasProperty (selected));
        };

        report ("set 1 property on each of N nodes, one batch", size, measureSeconds (undoable, selectAllTogether), size);
        report ("set 1 property on each of N nodes", size, measureSeconds (plain, selectAll), size);
        report ("set 1 property on each of N nodes, undoable", size, measureSeconds (undoable, selectAll), size);
    }
}

TEST_F (DataTreeBenchmarkTests, DISABLED_BuildingAndLoading)
{
    printHeader ("building and loading");

    const auto makeNothing = []
    {
        return DataTree();
    };

    for (const auto depth : { 5, 7 })
    {
        const auto numNodes = countNodes (depth);

        report ("build N nodes, children first", numNodes, measureSeconds (makeNothing, [depth] (DataTree& tree)
        {
            tree = buildBottomUp (depth);

            EXPECT_EQ (fanOut, tree.getNumChildren());
        }),
                numNodes);

        report ("build N nodes, parents first", numNodes, measureSeconds (makeNothing, [depth] (DataTree& tree)
        {
            tree = DataTree ("Node");
            fillTopDown (tree, depth);

            EXPECT_EQ (fanOut, tree.getNumChildren());
        }),
                numNodes);

        DataTree source ("Node");
        fillTopDown (source, depth);

        MemoryOutputStream stream;
        source.writeToBinaryStream (stream);

        const MemoryBlock binary (stream.getData(), stream.getDataSize());
        const auto xml = source.createXml();

        report ("read N nodes from binary", numNodes, measureSeconds (makeNothing, [&binary] (DataTree& tree)
        {
            MemoryInputStream input (binary, false);
            tree = DataTree::readFromBinaryStream (input);

            EXPECT_EQ (fanOut, tree.getNumChildren());
        }),
                numNodes);

        report ("read N nodes from XML", numNodes, measureSeconds (makeNothing, [&xml] (DataTree& tree)
        {
            tree = DataTree::fromXml (*xml);

            EXPECT_EQ (fanOut, tree.getNumChildren());
        }),
                numNodes);
    }
}

TEST_F (DataTreeBenchmarkTests, DISABLED_ObjectList)
{
    printHeader ("object list");

    for (const auto size : sizes)
    {
        const auto detached = [size]
        {
            return makeListed (size, false);
        };

        report ("append N children, list attached", size, measureSeconds (detached, [size] (Listed& listed)
        {
            {
                auto transaction = listed.family.parent.beginTransaction();

                for (const auto& child : listed.family.children)
                    transaction.addChild (child);
            }

            EXPECT_EQ (size, listed.list->getNumObjects());
        }),
                size);
    }

    // The list sorts itself on every move, so this one is kept small
    constexpr int numMoves = 8;

    for (const auto size : { 500, 2000 })
    {
        const auto attached = [size]
        {
            return makeListed (size, true);
        };

        report ("move 8 children of N, list attached", size, measureSeconds (attached, [size] (Listed& listed)
        {
            Random random (1);

            {
                auto transaction = listed.family.parent.beginTransaction();

                for (int i = 0; i < numMoves; ++i)
                    transaction.moveChild (random.nextInt (size), random.nextInt (size));
            }

            EXPECT_EQ (size, listed.list->getNumObjects());
        }),
                numMoves);
    }
}
