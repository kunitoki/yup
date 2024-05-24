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

namespace yup
{

//==============================================================================
/** Represents a 2D geometric path.

    The Path class encapsulates a series of geometric operations and shapes that can be described
    using lines, curves, and basic geometric shapes. It provides methods to construct and manipulate
    these paths with operations such as moving to a point, drawing lines, curves, rectangles,
    rounded rectangles, ellipses, and arcs. It supports both simple constructs such as lines and
    complex cubic Bezier curves.

    This class uses an internal storage mechanism to keep track of each segment in the path,
    allowing for efficient modifications and rendering. The Path can be used for drawing operations,
    hit testing, and bounding box calculations.
*/
class JUCE_API Path
{
public:
    //==============================================================================
    /** Defines the types of segments that can compose a path.

        Each segment in a path can be one of these types:
        - MoveTo: Move the starting point.
        - LineTo: Draw a straight line from the current point.
        - QuadTo: Draw a quadratic Bezier curve.
        - CubicTo: Draw a cubic Bezier curve.
    */
    enum SegmentType
    {
        MoveTo,
        LineTo,
        QuadTo,
        CubicTo
    };

    //==============================================================================
    /** Constructs an empty path. */
    Path() noexcept = default;

    /** Initializes a path and moves to the specified coordinates.

        This constructor creates a path and immediately moves the current point to
        the coordinates (x, y), starting a new sub-path.

        @param x The x-coordinate to move to.
        @param y The y-coordinate to move to.
    */
    Path (float x, float y) noexcept;

    /** Initializes a path and moves to the specified point.

        This constructor creates a path and immediately moves the current point to
        the given point, starting a new sub-path.

        @param p The point to move to.
    */
    Path (const Point<float>& p) noexcept;

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    Path (const Path& other) noexcept = default;
    Path (Path&& other) noexcept = default;
    Path& operator= (const Path& other) noexcept = default;
    Path& operator= (Path&& other) noexcept = default;

    //==============================================================================
    /** Reserves memory for a specified number of segments.

        This method allocates memory in advance for a number of segments, potentially improving
        performance by reducing the number of memory allocations required as segments are added.

        @param numSegments The number of segments for which to reserve space.
    */
    void reserveSpace (int numSegments);

    /** Returns the number of segments in the path.

        This method counts the total number of segments that have been added to the path,
        including all types of moves and draws.

        @return The number of segments in the path.
    */
    int size() const;

    //==============================================================================
    /** Clears all the segments from the path.

        This method removes all segments from the path, effectively resetting it to an empty state.
    */
    void clear();

    //==============================================================================
    /** Moves the current point to specified coordinates, starting a new sub-path.

        This command does not produce any visible output but updates the current point to
        the specified coordinates. It starts a new sub-path that subsequent commands are considered
        part of.

        @param x The x-coordinate to move to.
        @param y The y-coordinate to move to.
    */
    void moveTo (float x, float y);

    /** Moves the current point to a specified point, starting a new sub-path.

        This command does not produce any visible output but updates the current point to
        the specified point. It starts a new sub-path that subsequent commands are considered
        part of.

        @param p The point to move to.
    */
    void moveTo (const Point<float>& p);

    //==============================================================================
    /** Draws a line from the current point to specified coordinates.

        This method draws a straight line from the current point to the coordinates (x, y),
        then updates the current point to these coordinates.

        @param x The x-coordinate of the endpoint.
        @param y The y-coordinate of the endpoint.
    */
    void lineTo (float x, float y);

    /** Draws a line from the current point to a specified point.

        This method draws a straight line from the current point to the specified point,
        then updates the current point to this new location.

        @param p The point to draw the line to.
    */
    void lineTo (const Point<float>& p);

    //==============================================================================
    /** Draws a quadratic Bezier curve to specified coordinates with one control point.

        This method draws a quadratic Bezier curve from the current point to the point (x, y),
        using (x1, y1) as the control point. The current point is then set to (x, y).

        @param x The x-coordinate of the endpoint.
        @param y The y-coordinate of the endpoint.
        @param x1 The x-coordinate of the control point.
        @param y1 The y-coordinate of the control point.
    */
    void quadTo (float x, float y, float x1, float y1);

    /** Draws a quadratic Bezier curve to specified coordinates with one control point.

        This method draws a quadratic Bezier curve from the current point to the point (x, y),
        using the given point p as the control point. The current point is then set to (x, y).

        @param p The control point.
        @param x1 The x-coordinate of the endpoint.
        @param y1 The y-coordinate of the endpoint.
    */
    void quadTo (const Point<float>& p, float x1, float y1);

    //==============================================================================
    /** Draws a cubic Bezier curve to specified coordinates with two control points.

        This method draws a cubic Bezier curve from the current point to the point (x, y), using
        (x1, y1) and (x2, y2) as the control points. The current point is then updated to (x, y).

        @param x The x-coordinate of the endpoint.
        @param y The y-coordinate of the endpoint.
        @param x1 The x-coordinate of the first control point.
        @param y1 The y-coordinate of the first control point.
        @param x2 The x-coordinate of the second control point.
        @param y2 The y-coordinate of the second control point.
    */
    void cubicTo (float x, float y, float x1, float y1, float x2, float y2);

    /** Draws a cubic Bezier curve to specified coordinates with two control points.

        This method draws a cubic Bezier curve from the current point to the endpoint (x1, y1),
        using the given point p and (x2, y2) as the control points. The current point is then updated
        to (x1, y1).

        @param p The first control point.
        @param x1 The x-coordinate of the second control point.
        @param y1 The y-coordinate of the second control point.
        @param x2 The x-coordinate of the endpoint.
        @param y2 The y-coordinate of the endpoint.
    */
    void cubicTo (const Point<float>& p, float x1, float y1, float x2, float y2);

    //==============================================================================
    /** Closes the current sub-path by drawing a line to the start point of the sub-path.

        This method completes the current sub-path by drawing a line back to the starting point
        of the sub-path, effectively closing any open figures and making them ready for fill
        operations. This does not affect the current point.
    */
    void close();

    //==============================================================================
    /** Adds a straight line segment to the path between two points.

        This method appends a line from point p1 to point p2 to the path. This operation updates
        the current point to p2.

        @param p1 The starting point of the line.
        @param p2 The ending point of the line.
    */
    void addLine (const Point<float>& p1, const Point<float>& p2);

    /** Adds a line segment described by a Line object to the path.

        This method appends the line specified by the Line object to the path, updating
        the current point to the end of the line.

        @param line The line to add to the path.
    */
    void addLine (const Line<float>& line);

    //==============================================================================
    /** Adds a rectangle to the path.

        This method appends a rectangle with the specified position and size to the path. The rectangle
        is added as a closed sub-path, starting and ending at the top-left corner.

        @param x The x-coordinate of the top-left corner of the rectangle.
        @param y The y-coordinate of the top-left corner of the rectangle.
        @param width The width of the rectangle.
        @param height The height of the rectangle.
    */
    void addRectangle (float x, float y, float width, float height);

    /** Adds a rectangle described by a Rectangle object to the path.

        This method appends the rectangle specified by the Rectangle object to the path,
        adding it as a closed sub-path, starting and ending at the top-left corner of the rectangle.

        @param rect The rectangle to add to the path.
    */
    void addRectangle (const Rectangle<float>& rect);

    //==============================================================================
    /** Adds a rounded rectangle to the path.

        This method appends a rounded rectangle with specified position, size, and corner radii
        to the path. Each corner can have a different radius, allowing for complex shapes.
        The rounded rectangle is added as a closed sub-path.

        @param x The x-coordinate of the top-left corner.
        @param y The y-coordinate of the top-left corner.
        @param width The width of the rectangle.
        @param height The height of the rectangle.
        @param radiusTopLeft The radius of the top-left corner.
        @param radiusTopRight The radius of the top-right corner.
        @param radiusBottomLeft The radius of the bottom-left corner.
        @param radiusBottomRight The radius of the bottom-right corner.
    */
    void addRoundedRectangle (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);

    /** Adds a rounded rectangle described by a Rectangle object with specific corner radii to the path.

        This method appends a rounded rectangle specified by the Rectangle object and corner radii
        to the path. Each corner of the rectangle can have a different radius, which provides flexibility
        in the appearance of the corners. The rounded rectangle is added as a closed sub-path.

        @param rect The rectangle to which rounded corners are to be added.
        @param radiusTopLeft The radius of the top-left corner.
        @param radiusTopRight The radius of the top-right corner.
        @param radiusBottomLeft The radius of the bottom-left corner.
        @param radiusBottomRight The radius of the bottom-right corner.
    */
    void addRoundedRectangle (const Rectangle<float>& rect, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);

    //==============================================================================
    /** Adds an ellipse to the path.

        This method appends an ellipse with the specified position and size to the path.
        The ellipse is defined within a bounding rectangle, starting and ending at the
        rightmost point of the ellipse.

        @param x The x-coordinate of the top-left corner of the bounding rectangle.
        @param y The y-coordinate of the top-left corner of the bounding rectangle.
        @param width The width of the bounding rectangle.
        @param height The height of the bounding rectangle.
    */
    void addEllipse (float x, float y, float width, float height);

    /** Adds an ellipse described by a Rectangle object to the path.

        This method appends an ellipse defined within the bounding rectangle specified
        by the Rectangle object to the path. The ellipse starts and ends at the rightmost
        point of the ellipse, forming a complete and closed sub-path.

        @param rect The rectangle that bounds the ellipse.
    */
    void addEllipse (const Rectangle<float>& rect);

    //==============================================================================
    // TODO - doxygen
    void addCentredEllipse (float centerX, float centerY, float radiusX, float radiusY);

    // TODO - doxygen
    void addCentredEllipse (const Point<float>& center, float radiusX, float radiusY);

    //==============================================================================
    /** Adds an arc to the path.

        This method appends an arc defined within a bounding rectangle, between two radial angles.
        The arc can optionally start as a new sub-path or continue from the current point.

        @param x The x-coordinate of the top-left corner of the bounding rectangle.
        @param y The y-coordinate of the top-left corner of the bounding rectangle.
        @param width The width of the bounding rectangle.
        @param height The height of the bounding rectangle.
        @param fromRadians The starting angle of the arc, in radians.
        @param toRadians The ending angle of the arc, in radians.
        @param startAsNewSubPath Whether to start this as a new sub-path or continue from the current point.
    */
    void addArc (float x, float y, float width, float height,
                 float fromRadians, float toRadians,
                 bool startAsNewSubPath);

    /** Adds an arc described by a Rectangle object to the path.

        This method appends an arc that is defined within the bounding rectangle specified by
        the Rectangle object, between two radial angles. The arc can either start as a new sub-path
        or connect to the current point depending on the specified boolean.

        @param rect The rectangle that bounds the arc.
        @param fromRadians The starting angle of the arc, in radians.
        @param toRadians The ending angle of the arc, in radians.
        @param startAsNewSubPath Whether to start this as a new sub-path or continue from the current point.
    */
    void addArc (const Rectangle<float>& rect,
                 float fromRadians, float toRadians,
                 bool startAsNewSubPath);

    /** Adds a centered arc to the path.

        This method appends an arc centered at (centerX, centerY) with specified radii and rotation,
        between two radial angles. The arc can start as a new sub-path or continue from the current point.

        @param centerX The x-coordinate of the center of the arc.
        @param centerY The y-coordinate of the center of the arc.
        @param radiusX The horizontal radius of the arc.
        @param radiusY The vertical radius of the arc.
        @param rotationOfEllipse The rotation angle of the ellipse, in radians.
        @param fromRadians The starting angle of the arc, in radians.
        @param toRadians The ending angle of the arc, in radians.
        @param startAsNewSubPath Whether to start this as a new sub-path or continue from the current point.
    */
    void addCenteredArc (float centerX, float centerY, float radiusX, float radiusY,
                         float rotationOfEllipse, float fromRadians, float toRadians,
                         bool startAsNewSubPath);

    /** Adds a centered arc described by a Point object to the path.

        This method appends an arc centered at the specified point with given radii and rotation,
        between two radial angles. The arc can either start as a new sub-path or connect to the current point
        depending on the specified boolean.

        @param center The center point of the arc.
        @param radiusX The horizontal radius of the arc.
        @param radiusY The vertical radius of the arc.
        @param rotationOfEllipse The rotation angle of the ellipse, in radians.
        @param fromRadians The starting angle of the arc, in radians.
        @param toRadians The ending angle of the arc, in radians.
        @param startAsNewSubPath Whether to start this as a new sub-path or continue from the current point.
    */
    void addCenteredArc (const Point<float>& center, float radiusX, float radiusY,
                         float rotationOfEllipse, float fromRadians, float toRadians,
                         bool startAsNewSubPath);

    //==============================================================================
    /** Appends another path to this one.

        This method appends all segments of another path to this path. It effectively concatenates
        the other path onto this one, continuing from the current point of this path.

        @param other The path to append to this path.
    */
    void appendPath (const Path& other);

    /** Appends another path to this one applying a transformation.

        This method appends all segments of another path to this path applying an affine transformation to its point
        before being added.

        @param other The path with transformation to append to this path.
    */
    void appendPath (const Path& other, const AffineTransform& transform);

    //==============================================================================
    // TODO - doxygen
    Path& transform (const AffineTransform& t);

    // TODO - doxygen
    Path transformed (const AffineTransform& t) const;

    //==============================================================================
    /** Returns the bounding box of this path. */
    Rectangle<float> getBoundingBox() const;

    //==============================================================================
    // TODO - doxygen
    bool parsePathData (const String& pathData);

    //==============================================================================
    /** Provides an iterator to the beginning of the path data.

        This method returns an iterator pointing to the first segment in the path's internal data.
        It allows for iteration over the path's segments from the beginning to the end.

        @return An iterator to the beginning of the path data.
    */
    auto begin()
    {
        return data.begin();
    }

    /** Provides a constant iterator to the beginning of the path data.

        This method returns a constant iterator pointing to the first segment in the path's
        internal data. It ensures that the path data cannot be modified during the iteration.

        @return A constant iterator to the beginning of the path data.
    */
    auto begin() const
    {
        return data.begin();
    }

    /** Provides an iterator to the end of the path data.

        This method returns an iterator pointing just past the last segment in the path's internal data.
        It is used in conjunction with begin() for range-based iteration over the path's segments.

        @return An iterator to the end of the path data.
    */
    auto end()
    {
        return data.end();
    }

    /** Provides a constant iterator to the end of the path data.

        This method returns a constant iterator pointing just past the last segment in the path's
        internal data. It ensures safe iteration over the path segments without modifying them.

        @return A constant iterator to the end of the path data.
    */
    auto end() const
    {
        return data.end();
    }

    //==============================================================================
    struct Segment
    {
        constexpr Segment() noexcept = default;

        constexpr Segment (SegmentType type) noexcept
            : type (type)
        {
        }

        constexpr Segment (SegmentType type, float x, float y) noexcept
            : type (type)
            , x (x)
            , y (y)
        {
        }

        constexpr Segment (SegmentType type, float x, float y, float x1, float y1) noexcept
            : type (type)
            , x (x)
            , y (y)
            , x1 (x1)
            , y1 (y1)
        {
        }

        constexpr Segment (SegmentType type, float x, float y, float x1, float y1, float x2, float y2) noexcept
            : type (type)
            , x (x)
            , y (y)
            , x1 (x1)
            , y1 (y1)
            , x2 (x2)
            , y2 (y2)
        {
        }

        SegmentType type = SegmentType::MoveTo;
        float x = 0.0f, y = 0.0f;
        float x1 = 0.0f, y1 = 0.0f;
        float x2 = 0.0f, y2 = 0.0f;
    };

private:
    void updateBoundingBox (float x, float y);
    void resetBoundingBox();

    std::vector<Segment> data;
    int lastSubpathIndex = -1;
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::min();
};

} // namespace yup
