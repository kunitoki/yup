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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

class ComponentDocumentTests : public ::testing::Test
{
protected:
    void TearDown() override
    {
        if (file != File())
            file.deleteFile();
    }

    /** A document of one root, serving the assets it is given and keeping what it is told. */
    struct FakeDocument final : ComponentDocument
    {
        explicit FakeDocument (Component& rootToUse)
            : root (&rootToUse)
        {
            attachToRoot();
        }

        ~FakeDocument() override
        {
            detachFromRoot();
        }

        Component* getRoot() const override { return root; }

        const File& getBaseDirectory() const override { return baseDirectory; }

        std::optional<MemoryBlock> readAsset (const String& path) const override
        {
            if (const auto found = assets.find (path); found != assets.end())
                return MemoryBlock (found->second.toRawUTF8(), found->second.getNumBytesAsUTF8());

            return std::nullopt;
        }

        File getAssetFile (const String& path) const override
        {
            return baseDirectory != File() ? baseDirectory.getChildFile (path) : File();
        }

        std::shared_ptr<const DataTreeSourceMap> getSourceMap() const override { return nullptr; }

        DrawingScope::ComponentResolver getComponentResolver() const override { return nullptr; }

        void reportDiagnostic (DataTreeDiagnostic, const Component* = nullptr) override {}

        using ComponentDocument::advanceLoadGeneration;
        using ComponentDocument::attachToRoot;
        using ComponentDocument::notifyDocumentAssetChanged;
        using ComponentDocument::notifyDocumentLoaded;

        Component* root = nullptr;
        File baseDirectory;
        std::map<String, String> assets;
    };

    struct Recorder final : ComponentDocument::Listener
    {
        void documentLoaded (ComponentDocument& document) override { loaded.push_back (&document); }

        void documentAssetChanged (ComponentDocument&, const String& path) override { changed.add (path); }

        std::vector<ComponentDocument*> loaded;
        StringArray changed;
    };

    static String textOf (const std::optional<MemoryBlock>& bytes)
    {
        return bytes.has_value() ? bytes->toString() : String ("<nothing>");
    }

    File file;
};

TEST_F (ComponentDocumentTests, FindsTheDocumentOfItsRootAndOfTheComponentsInsideIt)
{
    Component root ("root");
    Component child ("child");
    Component grandChild ("grandChild");
    Component outside ("outside");
    root.addChildComponent (child);
    child.addChildComponent (grandChild);

    EXPECT_EQ (ComponentDocument::findFor (&root), nullptr);

    {
        FakeDocument document (root);

        EXPECT_EQ (ComponentDocument::findFor (&root), &document);
        EXPECT_EQ (ComponentDocument::findFor (&grandChild), &document);
        EXPECT_EQ (ComponentDocument::findFor (&outside), nullptr);
        EXPECT_EQ (ComponentDocument::findFor (nullptr), nullptr);
    }

    // A document that is gone is found no more
    EXPECT_EQ (ComponentDocument::findFor (&grandChild), nullptr);
}

TEST_F (ComponentDocumentTests, FindsTheClosestDocumentAboveAComponent)
{
    Component root ("root");
    Component panel ("panel");
    Component knob ("knob");
    root.addChildComponent (panel);
    panel.addChildComponent (knob);

    FakeDocument outer (root);
    FakeDocument inner (panel);

    EXPECT_EQ (ComponentDocument::findFor (&knob), &inner);
    EXPECT_EQ (ComponentDocument::findFor (&panel), &inner);
    EXPECT_EQ (ComponentDocument::findFor (&root), &outer);
}

TEST_F (ComponentDocumentTests, IsFoundFromTheRootItHasNowAndNoMoreFromTheOneBefore)
{
    Component first ("first");
    Component second ("second");
    Component child ("child");
    second.addChildComponent (child);

    FakeDocument document (first);
    EXPECT_EQ (ComponentDocument::findFor (&first), &document);

    document.root = &second;
    document.attachToRoot();

    EXPECT_EQ (ComponentDocument::findFor (&first), nullptr);
    EXPECT_EQ (ComponentDocument::findFor (&child), &document);

    // A root that is gone before its document leaves the document free for another one
    auto third = std::make_unique<Component> ("third");
    document.root = third.get();
    document.attachToRoot();
    EXPECT_EQ (ComponentDocument::findFor (&child), nullptr);

    third.reset();
    document.root = &first;
    document.attachToRoot();
    EXPECT_EQ (ComponentDocument::findFor (&first), &document);

    // Without a root it is found by no component
    document.root = nullptr;
    document.attachToRoot();
    EXPECT_EQ (ComponentDocument::findFor (&first), nullptr);
}

TEST_F (ComponentDocumentTests, ARootGivenToAnotherDocumentIsLeftToItByTheOneBefore)
{
    Component root ("root");

    auto earlier = std::make_unique<FakeDocument> (root);
    FakeDocument later (root);

    EXPECT_EQ (ComponentDocument::findFor (&root), &later);

    earlier.reset();
    EXPECT_EQ (ComponentDocument::findFor (&root), &later);
}

TEST_F (ComponentDocumentTests, ReadsAssetsThroughTheDocumentOfAComponentAndFromTheFileWithoutOne)
{
    file = File::createTempFile ("frag");
    ASSERT_TRUE (file.replaceWithText ("from the file"));

    Component root ("root");
    Component child ("child");
    Component alone ("alone");
    root.addChildComponent (child);

    FakeDocument document (root);
    document.assets["fx/crt.frag"] = "from the document";

    EXPECT_EQ (textOf (ComponentDocument::readAssetFor (&child, "fx/crt.frag")), "from the document");
    EXPECT_FALSE (ComponentDocument::readAssetFor (&child, "fx/missing.frag").has_value());

    // The document decides: what it does not have is not looked for in a file behind its back
    EXPECT_FALSE (ComponentDocument::readAssetFor (&child, file.getFullPathName()).has_value());

    // A component of no document reads the file by itself
    EXPECT_FALSE (ComponentDocument::readAssetFor (&alone, "fx/crt.frag").has_value());
    EXPECT_EQ (textOf (ComponentDocument::readAssetFor (&alone, file.getFullPathName())), "from the file");
    EXPECT_EQ (textOf (ComponentDocument::readAssetFor (nullptr, file.getFullPathName())), "from the file");
    EXPECT_FALSE (ComponentDocument::readAssetFor (nullptr, String()).has_value());
}

TEST_F (ComponentDocumentTests, TellsItsListenersUntilTheyAreRemoved)
{
    Component root ("root");
    FakeDocument document (root);
    Recorder recorder;

    document.addListener (&recorder);
    document.notifyDocumentLoaded();
    document.notifyDocumentAssetChanged ("fx/crt.frag");

    ASSERT_EQ (recorder.loaded.size(), 1u);
    EXPECT_EQ (recorder.loaded.front(), &document);
    EXPECT_EQ (recorder.changed.joinIntoString (", "), "fx/crt.frag");

    document.removeListener (&recorder);
    document.notifyDocumentLoaded();
    document.notifyDocumentAssetChanged ("fx/glow.frag");

    EXPECT_EQ (recorder.loaded.size(), 1u);
    EXPECT_EQ (recorder.changed.joinIntoString (", "), "fx/crt.frag");
}

TEST_F (ComponentDocumentTests, TheLoadGenerationIsAnotherOneEachTimeTheDocumentAdvancesIt)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    FakeDocument document (root);

    // What a component compares to know that the problems it told are forgotten
    const auto* found = ComponentDocument::findFor (&child);
    ASSERT_EQ (found, &document);
    EXPECT_EQ (found->getLoadGeneration(), 0u);

    // Telling the listeners is not what changes it
    document.notifyDocumentLoaded();
    document.notifyDocumentAssetChanged ("fx/crt.frag");
    EXPECT_EQ (found->getLoadGeneration(), 0u);

    document.advanceLoadGeneration();
    const auto first = found->getLoadGeneration();
    EXPECT_NE (first, 0u);

    document.advanceLoadGeneration();
    EXPECT_NE (found->getLoadGeneration(), first);
    EXPECT_NE (found->getLoadGeneration(), 0u);
}

TEST_F (ComponentDocumentTests, FindsTheFileOfAnAssetThroughTheDocumentOfAComponentAndByItselfWithoutOne)
{
    Component root ("root");
    Component child ("child");
    Component alone ("alone");
    root.addChildComponent (child);

    const auto folder = File::getSpecialLocation (File::tempDirectory);
    const auto absolute = folder.getChildFile ("fx/crt.frag").getFullPathName();

    FakeDocument document (root);

    // The document decides: its assets may be no files at all
    EXPECT_EQ (ComponentDocument::getAssetFileFor (&child, "fx/crt.frag"), File());

    document.baseDirectory = folder;
    EXPECT_EQ (ComponentDocument::getAssetFileFor (&child, "fx/crt.frag"), folder.getChildFile ("fx/crt.frag"));

    // A component of no document reads files: an absolute path as it is, a relative one from the working directory
    EXPECT_EQ (ComponentDocument::getAssetFileFor (&alone, "fx/crt.frag"), File::getCurrentWorkingDirectory().getChildFile ("fx/crt.frag"));
    EXPECT_EQ (ComponentDocument::getAssetFileFor (&alone, absolute), File (absolute));
    EXPECT_EQ (ComponentDocument::getAssetFileFor (nullptr, absolute), File (absolute));
    EXPECT_EQ (ComponentDocument::getAssetFileFor (nullptr, String()), File());
}

TEST_F (ComponentDocumentTests, AListenerFollowsOneDocumentAndLeavesItByItself)
{
    Component first ("first");
    Component second ("second");

    FakeDocument kept (first);
    auto gone = std::make_unique<FakeDocument> (second);

    {
        Recorder recorder;

        EXPECT_TRUE (recorder.follow (&kept));
        EXPECT_FALSE (recorder.follow (&kept));
        kept.notifyDocumentAssetChanged ("one");

        // Another document is followed in place of the one before
        EXPECT_TRUE (recorder.follow (gone.get()));
        kept.notifyDocumentAssetChanged ("two");
        gone->notifyDocumentAssetChanged ("three");

        EXPECT_EQ (recorder.changed.joinIntoString (", "), "one, three");

        // A document that went first is not looked for, and leaves the listener free for another one
        gone.reset();
        EXPECT_FALSE (recorder.follow (nullptr));
        EXPECT_TRUE (recorder.follow (&kept));

        kept.notifyDocumentAssetChanged ("four");
        EXPECT_EQ (recorder.changed.joinIntoString (", "), "one, three, four");
    }

    // The listener went while following: the document tells no one
    kept.notifyDocumentAssetChanged ("five");
    kept.notifyDocumentLoaded();
}
