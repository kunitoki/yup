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

#pragma once

#include <yup_data_model/yup_data_model.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <utility>
#include <vector>

using namespace yup;

//==============================================================================
/** Machinery for the tests that hold DataTree to its contract: data, notifications,
    their order and side effects.

    Nodes are told apart by their type, which is unique within a World, so a tree can be
    written down as text and two worlds built the same way can be compared.
*/
class DataTreeContractHarness : public ::testing::Test
{
protected:
    //==============================================================================
    static String label (const DataTree& tree)
    {
        return tree.isValid() ? tree.getType().toString() : String ("-");
    }

    static String describeValue (const var& value)
    {
        if (value.isVoid())
            return "void";

        if (value.isUndefined())
            return "undefined";

        if (value.isBool())
            return "bool:" + String (static_cast<bool> (value) ? "true" : "false");

        if (value.isInt())
            return "int:" + String (static_cast<int> (value));

        if (value.isInt64())
            return "int64:" + String (static_cast<int64> (value));

        if (value.isDouble())
            return "double:" + String (static_cast<double> (value), 3);

        if (value.isString())
            return "string:" + value.toString();

        return "other:" + value.toString();
    }

    static void expectLines (const StringArray& expected, const StringArray& actual, const char* what)
    {
        EXPECT_EQ (expected.joinIntoString ("\n").toStdString(), actual.joinIntoString ("\n").toStdString()) << what;
    }

    static UndoManager::Ptr makeUndoManager()
    {
        // No grouping timer, so the undo steps are only the ones the tests begin
        return UndoManager::Ptr (new UndoManager (RelativeTime()));
    }

    //==============================================================================
    /** The state of a node: its parent, its children in order and its properties by name. */
    struct NodeState
    {
        String parent;
        StringArray children;
        std::map<String, String> properties;
    };

    using Model = std::map<String, NodeState>;

    static String describeModel (const Model& model)
    {
        StringArray lines;

        for (const auto& [name, state] : model)
        {
            StringArray properties;

            for (const auto& [property, value] : state.properties)
                properties.add (property + "=" + value);

            lines.add (name + " ^" + state.parent
                       + " {" + properties.joinIntoString (" ") + "}"
                       + " [" + state.children.joinIntoString (" ") + "]");
        }

        return lines.joinIntoString ("\n");
    }

    //==============================================================================
    /** Every node of a test, attached or not, in creation order. */
    struct World
    {
        DataTree add (const String& name, DataTree parent = {})
        {
            auto node = DataTree (Identifier (name));

            if (parent.isValid())
            {
                auto transaction = parent.beginTransaction();
                transaction.addChild (node);
            }

            nodes.push_back (node);
            return node;
        }

        static void set (DataTree node, const Identifier& property, const var& value)
        {
            auto transaction = node.beginTransaction();
            transaction.setProperty (property, value);
        }

        DataTree find (const String& name) const
        {
            for (const auto& node : nodes)
            {
                if (node.getType().toString() == name)
                    return node;
            }

            return {};
        }

        /** Returns the node at an index, or an invalid tree when there is none. */
        DataTree at (int index) const
        {
            return isPositiveAndBelow (index, static_cast<int> (nodes.size())) ? nodes[static_cast<size_t> (index)] : DataTree();
        }

        /** Writes the trees down on one line, as `root{a=int:1}[child[grandchild] child] orphan`. */
        String describe() const
        {
            StringArray trees;

            for (const auto& node : nodes)
            {
                if (! node.getParent().isValid())
                    trees.add (describeNode (node, 0));
            }

            return trees.joinIntoString (" ");
        }

        Model capture() const
        {
            Model model;

            for (const auto& node : nodes)
            {
                NodeState state;
                state.parent = label (node.getParent());

                for (int i = 0; i < node.getNumChildren(); ++i)
                    state.children.add (label (node.getChild (i)));

                for (int i = 0; i < node.getNumProperties(); ++i)
                {
                    const auto property = node.getPropertyName (i);
                    state.properties[property.toString()] = describeValue (node.getProperty (property));
                }

                model[label (node)] = state;
            }

            return model;
        }

        /** Returns what is wrong with the structure: nothing when every child is held once,
            by the node it names as its parent, and no node is its own ancestor.
        */
        StringArray findStructureProblems() const
        {
            StringArray problems;
            std::map<String, int> timesHeld;

            for (const auto& node : nodes)
            {
                for (int i = 0; i < node.getNumChildren(); ++i)
                {
                    const auto child = node.getChild (i);
                    const auto childName = label (child);

                    if (find (childName) != child)
                        problems.add (label (node) + " holds " + childName + ", which is not the node of that name");

                    if (child.getParent() != node)
                        problems.add (label (node) + " holds " + childName + ", whose parent is " + label (child.getParent()));

                    ++timesHeld[childName];
                }
            }

            for (const auto& node : nodes)
            {
                const auto name = label (node);
                const auto held = timesHeld[name];

                if (held > 1)
                    problems.add (name + " is held " + String (held) + " times");

                if (node.getParent().isValid() && held == 0)
                    problems.add (name + " has the parent " + label (node.getParent()) + ", which does not hold it");

                if (hasParentCycle (node, static_cast<int> (nodes.size())))
                    problems.add (name + " is its own ancestor");
            }

            return problems;
        }

        std::vector<DataTree> nodes;

    private:
        String describeNode (const DataTree& node, int depth) const
        {
            if (depth > static_cast<int> (nodes.size()))
                return "<cycle>";

            auto text = node.getType().toString();

            StringArray properties;

            for (int i = 0; i < node.getNumProperties(); ++i)
            {
                const auto property = node.getPropertyName (i);
                properties.add (property.toString() + "=" + describeValue (node.getProperty (property)));
            }

            if (! properties.isEmpty())
            {
                properties.sort (false);
                text += "{" + properties.joinIntoString (" ") + "}";
            }

            StringArray children;

            for (int i = 0; i < node.getNumChildren(); ++i)
                children.add (describeNode (node.getChild (i), depth + 1));

            if (! children.isEmpty())
                text += "[" + children.joinIntoString (" ") + "]";

            return text;
        }

        static bool hasParentCycle (const DataTree& node, int stepsLeft)
        {
            const auto parent = node.getParent();

            if (! parent.isValid())
                return false;

            return stepsLeft == 0 || hasParentCycle (parent, stepsLeft - 1);
        }
    };

    //==============================================================================
    /** Writes down every callback, in order, with the node whose listeners received it.

        One listener per node, so the receiver is known whatever the arguments are.
    */
    class Recorder
    {
    public:
        struct Event
        {
            String receiver;
            String text;
            bool isChange = true;
        };

        Recorder() = default;

        void watch (const DataTree& node)
        {
            probes.push_back (std::make_unique<Probe> (*this, node));
        }

        void watch (const World& world)
        {
            for (const auto& node : world.nodes)
                watch (node);
        }

        /** Returns the callbacks of the changes received since the last call. */
        StringArray take()
        {
            StringArray texts;

            for (const auto& event : std::exchange (events, {}))
            {
                if (event.isChange)
                    texts.add (event.text);
            }

            return texts;
        }

        /** Returns the callbacks received since the last call, with the start and end of the transactions. */
        StringArray takeAll()
        {
            StringArray texts;

            for (const auto& event : std::exchange (events, {}))
                texts.add (event.text);

            return texts;
        }

        /** Returns the callbacks received since the last call, each with the node that received it. */
        std::vector<Event> takeEvents()
        {
            return std::exchange (events, {});
        }

        /** When set, what it returns is appended to each callback, to pin what a callback can see. */
        std::function<String()> observe;

        /** Callbacks that reached the listeners of a node they are not about, or that a transaction
            should have been around, and transactions that ended without having started.
        */
        StringArray problems;

        /** The transactions that started and did not end yet. */
        int numOpenTransactions = 0;

    private:
        struct Probe final : DataTree::Listener
        {
            Probe (Recorder& ownerToUse, const DataTree& nodeToWatch)
                : owner (ownerToUse)
                , node (nodeToWatch)
                , name (label (nodeToWatch))
            {
                node.addListener (this);
            }

            ~Probe() override
            {
                node.removeListener (this);
            }

            void transactionStarted (DataTree& tree) override
            {
                ++numOpen;
                ++owner.numOpenTransactions;
                owner.record (name, tree, "transactionStarted " + label (tree), false);
            }

            void transactionEnded (DataTree& tree, DataTree::TransactionResult result) override
            {
                if (numOpen == 0)
                {
                    owner.problems.add ("transactionEnded " + name + " without a transactionStarted");
                }
                else
                {
                    --numOpen;
                    --owner.numOpenTransactions;
                }

                owner.record (name, tree, "transactionEnded " + label (tree) + (result == DataTree::TransactionResult::committed ? " committed" : " aborted"), false);
            }

            void propertyChanged (DataTree& tree, const Identifier& property) override
            {
                expectTransaction ("propertyChanged");
                owner.record (name, tree, "propertyChanged " + label (tree) + " " + property.toString());
            }

            void childAdded (DataTree& parent, DataTree& child) override
            {
                owner.record (name, parent, "childAdded " + label (parent) + " " + label (child));
            }

            void childRemoved (DataTree& parent, DataTree& child, int formerIndex) override
            {
                owner.record (name, parent, "childRemoved " + label (parent) + " " + label (child) + " " + String (formerIndex));
            }

            void childMoved (DataTree& parent, DataTree& child, int oldIndex, int newIndex) override
            {
                expectTransaction ("childMoved");
                owner.record (name, parent, "childMoved " + label (parent) + " " + label (child) + " " + String (oldIndex) + " " + String (newIndex));
            }

            void parentChanged (DataTree& child, DataTree& previousParent) override
            {
                owner.record (name, child, "parentChanged " + label (child) + " " + label (previousParent));
            }

            void ancestorChanged (DataTree& tree, DataTree& reparentedAncestor) override
            {
                owner.record (name, tree, "ancestorChanged " + label (tree) + " " + label (reparentedAncestor));
            }

            // These only ever come from a transaction that edits the node
            void expectTransaction (const char* callback)
            {
                if (numOpen == 0)
                    owner.problems.add (String (callback) + " " + name + " outside a transaction");
            }

            Recorder& owner;
            DataTree node;
            String name;
            int numOpen = 0;
        };

        void record (const String& receiver, const DataTree& subject, const String& text, bool isChange = true)
        {
            if (label (subject) != receiver)
                problems.add (text + " reached the listeners of " + receiver);

            events.push_back ({ receiver, observe != nullptr ? text + " | " + observe() : text, isChange });
        }

        std::vector<Event> events;
        std::vector<std::unique_ptr<Probe>> probes;

        YUP_DECLARE_NON_COPYABLE (Recorder)
    };

    //==============================================================================
    /** Rebuilds the world from the callbacks alone, the way a view following a tree does. */
    class Mirror final : public DataTree::Listener
    {
    public:
        explicit Mirror (World& worldToMirror)
            : model (worldToMirror.capture())
            , world (worldToMirror)
        {
            for (auto& node : world.nodes)
                node.addListener (this);
        }

        ~Mirror() override
        {
            for (auto& node : world.nodes)
                node.removeListener (this);
        }

        void propertyChanged (DataTree& tree, const Identifier& property) override
        {
            auto& properties = model[label (tree)].properties;

            if (tree.hasProperty (property))
                properties[property.toString()] = describeValue (tree.getProperty (property));
            else
                properties.erase (property.toString());
        }

        void childAdded (DataTree& parent, DataTree& child) override
        {
            auto& children = model[label (parent)].children;
            const auto index = parent.indexOf (child);

            if (! isPositiveAndBelow (index, children.size() + 1))
            {
                problems.add ("childAdded " + label (parent) + " " + label (child) + ": no place for the index " + String (index));
                return;
            }

            children.insert (index, label (child));
        }

        void childRemoved (DataTree& parent, DataTree& child, int formerIndex) override
        {
            auto& children = model[label (parent)].children;

            if (! isPositiveAndBelow (formerIndex, children.size()) || children[formerIndex] != label (child))
            {
                problems.add ("childRemoved " + label (parent) + " " + label (child) + " " + String (formerIndex) + ": the child was not there");
                return;
            }

            children.remove (formerIndex);
        }

        void childMoved (DataTree& parent, DataTree& child, int oldIndex, int newIndex) override
        {
            auto& children = model[label (parent)].children;

            if (! isPositiveAndBelow (oldIndex, children.size())
                || ! isPositiveAndBelow (newIndex, children.size())
                || children[oldIndex] != label (child))
            {
                problems.add ("childMoved " + label (parent) + " " + label (child) + " " + String (oldIndex) + " " + String (newIndex) + ": the child was not there");
                return;
            }

            children.move (oldIndex, newIndex);
        }

        void parentChanged (DataTree& child, DataTree& previousParent) override
        {
            auto& parent = model[label (child)].parent;

            if (parent != label (previousParent))
                problems.add ("parentChanged " + label (child) + " " + label (previousParent) + ": the previous parent was " + parent);

            parent = label (child.getParent());
        }

        Model model;
        StringArray problems;

    private:
        World& world;

        YUP_DECLARE_NON_COPYABLE (Mirror)
    };

    /** Expects a valid structure, and callbacks that were complete, in a usable order and sent to the right nodes. */
    static void expectConsistent (const World& world, const Mirror& mirror, const Recorder& recorder)
    {
        const auto structure = world.findStructureProblems();

        EXPECT_TRUE (structure.isEmpty()) << structure.joinIntoString ("\n");
        EXPECT_TRUE (mirror.problems.isEmpty()) << mirror.problems.joinIntoString ("\n");
        EXPECT_TRUE (recorder.problems.isEmpty()) << recorder.problems.joinIntoString ("\n");
        EXPECT_EQ (0, recorder.numOpenTransactions) << "transactions that did not end";

        EXPECT_EQ (describeModel (world.capture()).toStdString(), describeModel (mirror.model).toStdString())
            << "the callbacks do not replay to the tree";
    }

    //==============================================================================
    /** What a transaction is expected to do. */
    struct Expected
    {
        StringArray commit;   // the callbacks of the commit, and of a redo
        String committed;     // the world after it
        StringArray undo;     // the callbacks of undoing it
        bool undoable = true; // false when it must leave nothing to undo
    };

    /** Runs a transaction on a fresh world, without and with an UndoManager.

        `build (World&)` creates the nodes, `edit (World&, UndoManager*)` commits the transaction.
    */
    template <class Build, class Edit>
    static void expectGolden (Build&& build, Edit&& edit, const Expected& expected)
    {
        runGolden (build, edit, expected, false);
        runGolden (build, edit, expected, true);
    }

    //==============================================================================
    /** One edit of a script. Nodes are indices into the world, -1 being an invalid tree. */
    struct Op
    {
        enum class Kind
        {
            setProperty,
            removeProperty,
            removeAllProperties,
            addChild,
            removeChild,
            removeChildAt,
            removeAllChildren,
            moveChild,
            addChildren,
            removeChildRange,
            removeChildren,
            reorderChildren
        };

        Kind kind = Kind::setProperty;
        int node = -1;
        int first = 0;
        int second = 0;
        int property = 0;
        int value = 0;
        std::vector<int> nodes;
    };

    /** One transaction of a script. */
    struct Step
    {
        int target = 0;
        std::vector<Op> ops;
    };

    using Script = std::vector<Step>;

    static Identifier propertyAt (int index)
    {
        static const Identifier names[] = { "a", "b", "c", "d", "e", "f", "g", "h" };
        return names[index % 8];
    }

    static var valueAt (int index)
    {
        switch (index)
        {
            case 0:
                return 1;

            case 1:
                return 2;

            case 2:
                return 1.0;

            case 3:
                return "x";

            case 4:
                return true;

            default:
                return {};
        }
    }

    /** Fills a world with nodes named n0, n1, ..., some attached to earlier ones, some with properties. */
    static void buildRandomWorld (World& world, int64 seed, int numNodes)
    {
        Random random (seed);

        for (int i = 0; i < numNodes; ++i)
        {
            const auto attached = i > 0 && random.nextInt (4) != 0;
            const auto node = world.add ("n" + String (i), attached ? world.at (random.nextInt (i)) : DataTree());

            for (int numProperties = random.nextInt (3); numProperties > 0; --numProperties)
            {
                const auto property = random.nextInt (3);
                World::set (node, propertyAt (property), valueAt (random.nextInt (6)));
            }
        }
    }

    /** Makes a script of valid and invalid edits: null trees, the node itself, its ancestors,
        children of other nodes and indices out of range.
    */
    static Script makeScript (int64 seed, int numNodes, int numSteps)
    {
        static constexpr Op::Kind kinds[] = {
            Op::Kind::setProperty,
            Op::Kind::setProperty,
            Op::Kind::setProperty,
            Op::Kind::removeProperty,
            Op::Kind::removeAllProperties,
            Op::Kind::addChild,
            Op::Kind::addChild,
            Op::Kind::addChild,
            Op::Kind::addChild,
            Op::Kind::removeChild,
            Op::Kind::removeChildAt,
            Op::Kind::removeChildAt,
            Op::Kind::removeAllChildren,
            Op::Kind::moveChild,
            Op::Kind::moveChild,
            Op::Kind::addChildren,
            Op::Kind::addChildren,
            Op::Kind::removeChildRange,
            Op::Kind::removeChildren,
            Op::Kind::reorderChildren,
            Op::Kind::reorderChildren
        };

        Random random (seed);
        Script script;

        for (int stepIndex = 0; stepIndex < numSteps; ++stepIndex)
        {
            Step step;
            step.target = random.nextInt (numNodes);

            // Mostly short transactions, and some long enough to index their pending changes
            for (int numOps = random.nextInt (5) == 0 ? 20 + random.nextInt (30) : 1 + random.nextInt (4); numOps > 0; --numOps)
            {
                Op op;
                op.kind = kinds[random.nextInt (static_cast<int> (std::size (kinds)))];
                op.node = random.nextInt (numNodes + 1) - 1;
                op.first = random.nextInt (8) - 2;
                op.second = random.nextInt (8) - 2;
                op.property = random.nextInt (8);
                op.value = random.nextInt (6);

                for (int numListed = random.nextInt (6); numListed > 0; --numListed)
                    op.nodes.push_back (random.nextInt (numNodes + 1) - 1);

                step.ops.push_back (op);
            }

            script.push_back (step);
        }

        return script;
    }

    static void record (DataTree::Transaction& transaction, const World& world, const DataTree& target, const Op& op)
    {
        switch (op.kind)
        {
            case Op::Kind::setProperty:
                transaction.setProperty (propertyAt (op.property), valueAt (op.value));
                break;

            case Op::Kind::removeProperty:
                transaction.removeProperty (propertyAt (op.property));
                break;

            case Op::Kind::removeAllProperties:
                transaction.removeAllProperties();
                break;

            case Op::Kind::addChild:
                transaction.addChild (world.at (op.node), op.first);
                break;

            case Op::Kind::removeChild:
                transaction.removeChild (world.at (op.node));
                break;

            case Op::Kind::removeChildAt:
                transaction.removeChild (op.first);
                break;

            case Op::Kind::removeAllChildren:
                transaction.removeAllChildren();
                break;

            case Op::Kind::moveChild:
                transaction.moveChild (op.first, op.second);
                break;

            case Op::Kind::addChildren:
                transaction.addChildren (listedTrees (world, op), op.first);
                break;

            case Op::Kind::removeChildRange:
                transaction.removeChildren (op.first, op.second);
                break;

            case Op::Kind::removeChildren:
                transaction.removeChildren (listedTrees (world, op));
                break;

            case Op::Kind::reorderChildren:
                transaction.reorderChildren (reorderedChildren (target, op));
                break;
        }
    }

    /** Records an edit of several children as the single edits it is defined by. Only good for an
        edit that is alone in its transaction, as it reads the children the node has now.
    */
    static void recordAsSingleEdits (DataTree::Transaction& transaction, const World& world, const DataTree& target, const Op& op)
    {
        const auto listed = listedTrees (world, op);

        if (op.kind == Op::Kind::addChildren)
        {
            int numAdded = 0;

            for (auto it = listed.begin(); it != listed.end(); ++it)
            {
                if (! it->isValid() || *it == target || it->isAChildOf (target) || target.isAChildOf (*it) || std::find (listed.begin(), it, *it) != it)
                    continue;

                transaction.addChild (*it, op.first < 0 ? -1 : op.first + numAdded);
                ++numAdded;
            }
        }
        else if (op.kind == Op::Kind::removeChildRange)
        {
            for (int i = op.first + op.second; --i >= op.first;)
                transaction.removeChild (i);
        }
        else if (op.kind == Op::Kind::removeChildren)
        {
            int numFound = 0;

            for (int i = target.getNumChildren(); --i >= 0;)
            {
                const auto child = target.getChild (i);

                if (std::find (listed.begin(), listed.end(), child) != listed.end())
                {
                    transaction.removeChild (child);
                    ++numFound;
                }
            }

            // The listed trees that are not children are removals that remove nothing
            for (auto numLeft = static_cast<int> (listed.size()) - numFound; numLeft > 0; --numLeft)
                transaction.removeChild (DataTree());
        }
        else if (op.kind == Op::Kind::reorderChildren)
        {
            const auto order = reorderedChildren (target, op);
            const auto numChildren = static_cast<size_t> (target.getNumChildren());

            // Where each child is now, by the index it had to begin with
            std::vector<int> arrangement;
            auto isPermutation = order.size() == numChildren;

            for (size_t i = 0; i < numChildren; ++i)
            {
                arrangement.push_back (static_cast<int> (i));
                isPermutation = isPermutation && std::count (order.begin(), order.end(), target.getChild (static_cast<int> (i))) == 1;
            }

            int numMoves = 0;

            for (size_t i = 0; isPermutation && i < numChildren; ++i)
            {
                const auto from = std::find (arrangement.begin(), arrangement.end(), target.indexOf (order[i])) - arrangement.begin();

                if (from == static_cast<std::ptrdiff_t> (i))
                    continue;

                transaction.moveChild (static_cast<int> (from), static_cast<int> (i));
                arrangement.insert (arrangement.begin() + static_cast<std::ptrdiff_t> (i), arrangement[static_cast<size_t> (from)]);
                arrangement.erase (arrangement.begin() + from + 1);
                ++numMoves;
            }

            // An order that moves nothing still leaves an edit that does nothing
            if (numMoves == 0)
                transaction.moveChild (-1, -2);
        }
        else
        {
            record (transaction, world, target, op);
        }
    }

    /** Commits a step as one transaction. */
    static void apply (const World& world, const Step& step, UndoManager* undoManager)
    {
        const auto target = world.at (step.target);
        auto transaction = world.at (step.target).beginTransaction (undoManager);

        for (const auto& op : step.ops)
            record (transaction, world, target, op);
    }

    /** Commits several steps as one batch. */
    static void applyAsBatch (const World& world, const Script& steps, UndoManager* undoManager)
    {
        DataTree::BatchTransaction batch (undoManager);

        for (const auto& step : steps)
        {
            const auto target = world.at (step.target);
            auto& transaction = batch.edit (target);

            for (const auto& op : step.ops)
                record (transaction, world, target, op);
        }
    }

    /** Records several steps on one transaction per node, then commits them in the order the nodes were first edited. */
    static void applyAsTransactions (const World& world, const Script& steps, UndoManager* undoManager)
    {
        std::vector<int> targets;
        std::vector<DataTree::Transaction> transactions;

        for (const auto& step : steps)
        {
            auto found = static_cast<size_t> (std::find (targets.begin(), targets.end(), step.target) - targets.begin());

            if (found == targets.size())
            {
                targets.push_back (step.target);
                transactions.push_back (world.at (step.target).beginTransaction (undoManager));
            }

            for (const auto& op : step.ops)
                record (transactions[found], world, world.at (step.target), op);
        }

        for (auto& transaction : transactions)
            transaction.commit();
    }

    /** Commits a step as one transaction of single edits. */
    static void applyAsSingleEdits (const World& world, const Step& step, UndoManager* undoManager)
    {
        const auto target = world.at (step.target);
        auto transaction = world.at (step.target).beginTransaction (undoManager);

        for (const auto& op : step.ops)
            recordAsSingleEdits (transaction, world, target, op);
    }

    static String describeStep (const Step& step)
    {
        StringArray ops;

        for (const auto& op : step.ops)
        {
            switch (op.kind)
            {
                case Op::Kind::setProperty:
                    ops.add ("setProperty (" + propertyAt (op.property).toString() + ", " + describeValue (valueAt (op.value)) + ")");
                    break;

                case Op::Kind::removeProperty:
                    ops.add ("removeProperty (" + propertyAt (op.property).toString() + ")");
                    break;

                case Op::Kind::removeAllProperties:
                    ops.add ("removeAllProperties()");
                    break;

                case Op::Kind::addChild:
                    ops.add ("addChild (" + describeNodeIndex (op.node) + ", " + String (op.first) + ")");
                    break;

                case Op::Kind::removeChild:
                    ops.add ("removeChild (" + describeNodeIndex (op.node) + ")");
                    break;

                case Op::Kind::removeChildAt:
                    ops.add ("removeChild (" + String (op.first) + ")");
                    break;

                case Op::Kind::removeAllChildren:
                    ops.add ("removeAllChildren()");
                    break;

                case Op::Kind::moveChild:
                    ops.add ("moveChild (" + String (op.first) + ", " + String (op.second) + ")");
                    break;

                case Op::Kind::addChildren:
                    ops.add ("addChildren ({" + describeListed (op) + "}, " + String (op.first) + ")");
                    break;

                case Op::Kind::removeChildRange:
                    ops.add ("removeChildren (" + String (op.first) + ", " + String (op.second) + ")");
                    break;

                case Op::Kind::removeChildren:
                    ops.add ("removeChildren ({" + describeListed (op) + "})");
                    break;

                case Op::Kind::reorderChildren:
                    ops.add ("reorderChildren (" + String (op.value) + ", " + String (op.second) + ")");
                    break;
            }
        }

        return "n" + String (step.target) + ": " + ops.joinIntoString ("; ");
    }

private:
    static String describeNodeIndex (int index)
    {
        return index < 0 ? String ("-") : "n" + String (index);
    }

    static String describeListed (const Op& op)
    {
        StringArray names;

        for (const auto node : op.nodes)
            names.add (describeNodeIndex (node));

        return names.joinIntoString (" ");
    }

    /** Returns the children of a node shuffled, and once in a while not quite all of them, or one twice. */
    static std::vector<DataTree> reorderedChildren (const DataTree& target, const Op& op)
    {
        std::vector<int> indices;

        for (int i = 0; i < target.getNumChildren(); ++i)
            indices.push_back (i);

        Random random (op.value * 8 + op.first);

        for (auto i = indices.size(); i > 1; --i)
            std::swap (indices[i - 1], indices[static_cast<size_t> (random.nextInt (static_cast<int> (i)))]);

        if (op.second == 0 && ! indices.empty())
            indices.pop_back();
        else if (op.second == 1 && ! indices.empty())
            indices.back() = indices.front();

        std::vector<DataTree> order;

        for (const auto index : indices)
            order.push_back (target.getChild (index));

        return order;
    }

    static std::vector<DataTree> listedTrees (const World& world, const Op& op)
    {
        std::vector<DataTree> trees;

        for (const auto node : op.nodes)
            trees.push_back (world.at (node));

        return trees;
    }

    template <class Build, class Edit>
    static void runGolden (Build& build, Edit& edit, const Expected& expected, bool useUndoManager)
    {
        SCOPED_TRACE (useUndoManager ? "with an UndoManager" : "without an UndoManager");

        World world;
        build (world);

        const auto initial = world.describe();

        Recorder recorder;
        recorder.watch (world);

        Mirror mirror (world);

        const auto undoManager = useUndoManager ? makeUndoManager() : UndoManager::Ptr();

        edit (world, undoManager.get());

        expectLines (expected.commit, recorder.take(), "callbacks of the commit");
        EXPECT_EQ (expected.committed, world.describe());
        expectConsistent (world, mirror, recorder);

        if (undoManager == nullptr)
            return;

        EXPECT_EQ (expected.undoable, undoManager->canUndo());

        if (! undoManager->canUndo())
            return;

        EXPECT_TRUE (undoManager->undo());
        expectLines (expected.undo, recorder.take(), "callbacks of the undo");
        EXPECT_EQ (initial, world.describe());
        expectConsistent (world, mirror, recorder);

        EXPECT_TRUE (undoManager->redo());
        expectLines (expected.commit, recorder.take(), "callbacks of the redo");
        EXPECT_EQ (expected.committed, world.describe());
        expectConsistent (world, mirror, recorder);
    }
};
