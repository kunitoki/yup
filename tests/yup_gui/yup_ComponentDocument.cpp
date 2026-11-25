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
            registerDocument();
        }

        ~FakeDocument() override
        {
            unregisterDocument();
        }

        Component* getRoot() const override { return root; }

        const File& getBaseDirectory() const override { return baseDirectory; }

        std::optional<MemoryBlock> readAsset (const String& path) const override
        {
            if (const auto found = assets.find (path); found != assets.end())
                return MemoryBlock (found->second.toRawUTF8(), found->second.getNumBytesAsUTF8());

            return std::nullopt;
        }

        File getAssetFile (const String&) const override { return {}; }

        std::shared_ptr<const DataTreeSourceMap> getSourceMap() const override { return nullptr; }

        DrawingScope::ComponentResolver getComponentResolver() const override { return nullptr; }

        void reportDiagnostic (DataTreeDiagnostic, const Component* = nullptr) override {}

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
