#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

// Named differently from the other drag-drop helpers: the test sources are compiled together, so two
// anonymous-namespace classes of the same name would clash.
class SourceTestComponent : public Component
    , public DragAndDropSource
{
public:
    using Component::Component;
};

} // namespace

/** Covers the half of the drag-and-drop API that a drag is *started* from.

    Everything here works without a window, a mouse gesture or the ghost, which is what keeps it worth
    testing: the options a caller builds, the invariants those builders maintain, and the refusals
    that stop a drag before any window is involved. */
class DragAndDropSourceTests : public ::testing::Test
{
protected:
    DragAndDropManager& manager() const
    {
        return *DragAndDropManager::getInstance();
    }

    SourceTestComponent source;
};

TEST_F (DragAndDropSourceTests, DefaultOptionsDescribeAnEmptyInternalDrag)
{
    const DragAndDropSource::DragOptions options;

    EXPECT_TRUE (options.data.isEmpty());
    EXPECT_EQ (nullptr, options.dragImageComponent);
    EXPECT_FALSE (options.dragImage.isValid());
    EXPECT_EQ (0.0f, options.imageOffset.getX());
    EXPECT_EQ (0.0f, options.imageOffset.getY());
    EXPECT_FLOAT_EQ (0.7f, options.imageOpacity);
    EXPECT_FALSE (options.allowExternalDrag);
}

TEST_F (DragAndDropSourceTests, BuildersChainAndPreserveWhatTheySet)
{
    const auto options = DragAndDropSource::DragOptions{}
                             .withData (DragAndDropData{}.withText ("kick"))
                             .withImageOpacity (0.25f)
                             .withExternalDragAllowed (true);

    EXPECT_TRUE (options.data.hasText());
    EXPECT_EQ ("kick", options.data.getText());
    EXPECT_FLOAT_EQ (0.25f, options.imageOpacity);
    EXPECT_TRUE (options.allowExternalDrag);
}

TEST_F (DragAndDropSourceTests, WithDragImageRecordsTheImageAndItsOffset)
{
    const auto options = DragAndDropSource::DragOptions{}
                             .withDragImage (Image(), Point<float> (12.0f, 7.0f));

    EXPECT_EQ (12.0f, options.imageOffset.getX());
    EXPECT_EQ (7.0f, options.imageOffset.getY());
    EXPECT_EQ (nullptr, options.dragImageComponent);
}

TEST_F (DragAndDropSourceTests, WithDragImageClearsALiveComponent)
{
    // The builder takes no chances either way round: setting a static image drops any component that
    // was going to be hosted instead, so the two can never disagree about which one is the ghost.
    Component hosted;

    const auto options = DragAndDropSource::DragOptions{}
                             .withDragImageComponent (&hosted, Point<float> (4.0f, 4.0f))
                             .withDragImage (Image(), Point<float> (1.0f, 2.0f));

    EXPECT_EQ (nullptr, options.dragImageComponent);
    EXPECT_EQ (1.0f, options.imageOffset.getX());
}

TEST_F (DragAndDropSourceTests, WithDragImageComponentRecordsTheComponentAndItsOffset)
{
    Component hosted;

    const auto options = DragAndDropSource::DragOptions{}
                             .withDragImageComponent (&hosted, Point<float> (30.0f, 12.0f));

    EXPECT_EQ (&hosted, options.dragImageComponent);
    EXPECT_EQ (30.0f, options.imageOffset.getX());
    EXPECT_EQ (12.0f, options.imageOffset.getY());
}

TEST_F (DragAndDropSourceTests, AnEmptyPayloadCannotStartADrag)
{
    EXPECT_FALSE (source.startDragging (DragAndDropSource::DragOptions{}));

    // Refused before anything was created, so no session is left in flight for the next attempt.
    EXPECT_FALSE (source.isCurrentlyDragging());
    EXPECT_FALSE (manager().isDragging());
}

TEST_F (DragAndDropSourceTests, ARefusedDragReportsNothingToTheSource)
{
    int startedCount = 0;
    int endedCount = 0;

    source.onDragStarted = [&startedCount] (const DragAndDropData&) { ++startedCount; };
    source.onDragEnded = [&endedCount] (const DragAndDropData&, DragAndDropAction) { ++endedCount; };

    source.startDragging (DragAndDropSource::DragOptions{}.withImageOpacity (0.5f));

    // A drag that never started is not a drag that ended.
    EXPECT_EQ (0, startedCount);
    EXPECT_EQ (0, endedCount);
}

TEST_F (DragAndDropSourceTests, NothingIsDraggingWhileNoDragHasStarted)
{
    EXPECT_FALSE (source.isCurrentlyDragging());
    EXPECT_EQ (nullptr, manager().getCurrentDragSourceComponent());
}

TEST_F (DragAndDropSourceTests, TheSourceIsItsOwnComponent)
{
    EXPECT_EQ (static_cast<Component*> (&source), source.getDragSourceComponent());
}

TEST_F (DragAndDropSourceTests, AllowedActionsBuilderIsChainable)
{
    const auto options = DragAndDropSource::DragOptions{}
                             .withData (DragAndDropData{}.withText ("kick"))
                             .withAllowedActions (dragAndDropActionCopy);

    EXPECT_FALSE (options.data.isEmpty());
}

TEST_F (DragAndDropSourceTests, TheVirtualHooksDefaultToDoingNothing)
{
    // Called through the base so the defaults are the ones exercised: overriding either hook has to be
    // optional, and calling them must be harmless either way.
    DragAndDropSource& asSource = source;

    asSource.dragOperationStarted (DragAndDropData{}.withText ("kick"));
    asSource.dragOperationEnded (DragAndDropData{}.withText ("kick"), DragAndDropAction::none);

    SUCCEED();
}
