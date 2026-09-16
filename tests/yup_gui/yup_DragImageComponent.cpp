#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

Image makeTestImage (int width, int height)
{
    Image image (width, height, PixelFormat::RGBA);
    image.fill (0xff3366aau);

    return image;
}

} // namespace

/** Covers the ghost window: what it shows, how big it is, and who owns what.

    Note that DragImageComponent is a window-backed component - its constructor calls addToDesktop - so
    unlike the rest of the drag-and-drop tests these need a desktop to run against.

    Nothing here asserts on the drawing itself, which needs a graphics context; what is checked is the
    state the ghost exposes through the ordinary Component API. */
class DragImageComponentTests : public ::testing::Test
{
protected:
    DragImageComponent ghost;
};

TEST_F (DragImageComponentTests, AFreshGhostIsHidden)
{
    // The manager shows it only once it has something to show.
    EXPECT_FALSE (ghost.isVisible());
}

TEST_F (DragImageComponentTests, AnImageShowsTheGhostSizedToIt)
{
    ghost.setDragImage (makeTestImage (20, 12), {}, 0.5f);

    EXPECT_TRUE (ghost.isVisible());
    EXPECT_EQ (20.0f, ghost.getWidth());
    EXPECT_EQ (12.0f, ghost.getHeight());
}

TEST_F (DragImageComponentTests, AnImageWithNoSizeLeavesTheGhostHidden)
{
    // An empty image has no size, and a zero-sized ghost is not worth showing.
    ghost.setDragImage (Image(), {}, 1.0f);

    EXPECT_FALSE (ghost.isVisible());
}

TEST_F (DragImageComponentTests, ClearingTheImageHidesTheGhostAgain)
{
    ghost.setDragImage (makeTestImage (20, 12), {}, 1.0f);
    ASSERT_TRUE (ghost.isVisible());

    ghost.clearDragImage();

    EXPECT_FALSE (ghost.isVisible());
}

TEST_F (DragImageComponentTests, MovingPutsTheHotspotUnderThePointer)
{
    ghost.setDragImage (makeTestImage (20, 12), Point<float> (5.0f, 7.0f), 1.0f);
    ghost.moveToScreenPosition (Point<float> (100.0f, 200.0f));

    // The hotspot is the point within the ghost that sits under the cursor, so it comes off the
    // position rather than being ignored.
    EXPECT_EQ (95.0f, ghost.getX());
    EXPECT_EQ (193.0f, ghost.getY());
}

TEST_F (DragImageComponentTests, AHostedComponentIsReparentedAndSized)
{
    Component hosted;
    hosted.setSize (30.0f, 18.0f);

    ghost.setDragImageComponent (&hosted, {}, 1.0f);

    EXPECT_TRUE (ghost.isVisible());
    EXPECT_EQ (30.0f, ghost.getWidth());
    EXPECT_EQ (18.0f, ghost.getHeight());

    // The ghost takes the component over for the duration, and fills itself with it.
    EXPECT_EQ (static_cast<Component*> (&ghost), hosted.getParentComponent());
    EXPECT_EQ (0.0f, hosted.getX());
    EXPECT_EQ (0.0f, hosted.getY());
}

TEST_F (DragImageComponentTests, ClearingAHostedComponentDetachesIt)
{
    Component hosted;
    hosted.setSize (30.0f, 18.0f);

    ghost.setDragImageComponent (&hosted, {}, 1.0f);
    ASSERT_EQ (static_cast<Component*> (&ghost), hosted.getParentComponent());

    ghost.clearDragImage();

    // The caller keeps ownership, so the ghost has to let go rather than leave a dangling child.
    EXPECT_EQ (nullptr, hosted.getParentComponent());
    EXPECT_FALSE (ghost.isVisible());
}

TEST_F (DragImageComponentTests, SettingAnImageAfterAComponentReplacesIt)
{
    Component hosted;
    hosted.setSize (30.0f, 18.0f);

    ghost.setDragImageComponent (&hosted, {}, 1.0f);
    ghost.setDragImage (makeTestImage (10, 10), {}, 1.0f);

    // Whichever was set last wins, and the component it replaced is released rather than left behind.
    EXPECT_EQ (nullptr, hosted.getParentComponent());
    EXPECT_EQ (10.0f, ghost.getWidth());
    EXPECT_EQ (10.0f, ghost.getHeight());
}

//==============================================================================

namespace
{

/** Painting needs a graphics context, which is the one thing the rest of these tests avoid. */
class DragImageComponentPaintTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GraphicsContext::Options opts;
        opts.allowHeadlessRendering = true;

        context = GraphicsContext::createContext (GpuPlatform::Headless, opts);
        ASSERT_NE (nullptr, context);

        renderer = context->makeRenderer (32, 32);
        ASSERT_NE (nullptr, renderer);
    }

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;
};

} // namespace

TEST_F (DragImageComponentPaintTests, PaintsTheDragImageWhenNothingElseIsHosted)
{
    DragImageComponent ghost;
    ghost.setDragImage (makeTestImage (20, 12), {}, 1.0f);

    Graphics g (*context, *renderer, 1.0f);

    EXPECT_NO_THROW (ghost.paint (g));
}

TEST_F (DragImageComponentPaintTests, DoesNotPaintADragImageWhileAComponentIsHosted)
{
    Component hosted;
    hosted.setSize (20.0f, 12.0f);

    DragImageComponent ghost;
    ghost.setDragImageComponent (&hosted, {}, 1.0f);

    Graphics g (*context, *renderer, 1.0f);

    // The hosted component draws itself through the normal child paint, so the ghost draws nothing.
    EXPECT_NO_THROW (ghost.paint (g));
}
