/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include "yup_YupGraphics_bindings.h"

#include "../utilities/yup_ClassDemangling.h"
#include "../utilities/yup_PythonInterop.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_STL
#include "../utilities/yup_PyBind11Includes.h"

#include <functional>
#include <memory>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

// clang-format off
// ============================================================================================

template <template <class> class Class, class... Types>
void registerPoint (py::module_& m)
{
    py::dict type;

    ([&]
    {
        using ValueType = Types;
        using T = Class<ValueType>;

        const auto className = Helpers::pythonizeCompoundClassName ("Point", typeid (Types).name());

        auto class_ = py::class_<T> (m, className.toRawUTF8())
            // Constructors
            .def (py::init<>())
            .def (py::init<ValueType, ValueType>())
            .def (py::init<const T&>())

            // Basic methods
            .def ("isOrigin", &T::isOrigin)
            .def ("isOnXAxis", &T::isOnXAxis)
            .def ("isOnYAxis", &T::isOnYAxis)
            .def ("getX", &T::getX)
            .def ("getY", &T::getY)
            .def ("setX", &T::setX)
            .def ("setY", &T::setY)
            .def ("withX", &T::withX)
            .def ("withY", &T::withY)
            .def ("withXY", &T::withXY)

            // Distance methods
            .def ("distanceTo", &T::distanceTo)
            .def ("distanceToSquared", &T::distanceToSquared)
            .def ("horizontalDistanceTo", &T::horizontalDistanceTo)
            .def ("verticalDistanceTo", &T::verticalDistanceTo)
            .def ("manhattanDistanceTo", &T::manhattanDistanceTo)

            // Vector operations
            .def ("magnitude", &T::magnitude)
            .def ("dotProduct", &T::dotProduct)
            .def ("crossProduct", &T::crossProduct)
            .def ("angleTo", &T::angleTo)
            .def ("normalize", &T::normalize)
            .def ("normalized", &T::normalized)
            .def ("isNormalized", &T::isNormalized)

            // Geometric operations
            .def ("translate", py::overload_cast<ValueType, ValueType> (&T::translate))
            .def ("translate", py::overload_cast<const T&> (&T::translate))
            .def ("translated", py::overload_cast<ValueType, ValueType> (&T::translated, py::const_))
            .def ("translated", py::overload_cast<const T&> (&T::translated, py::const_))
            .def ("rotateClockwise", &T::rotateClockwise)
            .def ("rotatedClockwise", &T::rotatedClockwise)
            .def ("rotateCounterClockwise", &T::rotateCounterClockwise)
            .def ("rotatedCounterClockwise", &T::rotatedCounterClockwise)

            // Utility methods
            .def ("midpoint", &T::midpoint)
            .def ("pointBetween", &T::pointBetween)
            .def ("isCollinear", &T::isCollinear)
            .def ("isWithinCircle", &T::isWithinCircle)
            .def ("isWithinRectangle", &T::isWithinRectangle)

            // Reflection methods
            .def ("reflectOverXAxis", &T::reflectOverXAxis)
            .def ("reflectedOverXAxis", &T::reflectedOverXAxis)
            .def ("reflectOverYAxis", &T::reflectOverYAxis)
            .def ("reflectedOverYAxis", &T::reflectedOverYAxis)
            .def ("reflectOverOrigin", &T::reflectOverOrigin)
            .def ("reflectedOverOrigin", &T::reflectedOverOrigin)

            // Math operations
            .def ("min", &T::min)
            .def ("max", &T::max)
            .def ("abs", &T::abs)
            .def ("lerp", &T::lerp)

            // Transformation
            .def ("transform", &T::transform)
            .def ("transformed", &T::transformed)

            // Comparison
            .def (py::self == py::self)
            .def (py::self != py::self)
            .def ("approximatelyEqualTo", &T::approximatelyEqualTo)

            // Operators
            .def (py::self + py::self)
            .def (py::self += py::self)
            .def (py::self - py::self)
            .def (py::self -= py::self)
            .def (py::self * py::self)
            .def (py::self *= py::self)
            .def (py::self / py::self)
            .def (py::self /= py::self)
            .def (py::self + ValueType())
            .def (py::self += ValueType())
            .def (py::self - ValueType())
            .def (py::self -= ValueType())
            .def (py::self * ValueType())
            .def (py::self *= ValueType())
            .def (py::self / ValueType())
            .def (py::self /= ValueType())
            .def (-py::self)

            // Add conversion methods
            .def ("toInt", [](const T& self) { return self.template to<int>(); })
            .def ("toLong", [](const T& self) { return self.template to<long>(); })
            .def ("toFloat", [](const T& self) { return self.template to<float>(); })
            .def ("toDouble", [](const T& self) { return self.template to<double>(); })

            .def ("__repr__", [](const T& self)
            {
                String result;
                result
                    << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                    << "(" << self.getX() << ", " << self.getY() << ")";
                return result;
            })
            //.def ("__str__", &T::toString)
            .def ("__str__", [](const T& self)
            {
                String result;
                result << self.getX() << ", " << self.getY();
                return result;
            })
        ;

        // Add floating-point specific methods
        if constexpr (std::is_floating_point_v<ValueType>)
        {
            class_
                .def ("getPointOnCircumference", py::overload_cast<float, float> (&T::template getPointOnCircumference<ValueType>, py::const_))
                .def ("getPointOnCircumference", py::overload_cast<float, float, float> (&T::template getPointOnCircumference<ValueType>, py::const_))
                .def ("isFinite", [](const T& self) { return self.template isFinite<ValueType>(); })
                .def ("floor", [](const T& self) { return self.template floor<ValueType>(); })
                .def ("ceil", [](const T& self) { return self.template ceil<ValueType>(); })
                .def ("scale", [](T& self, ValueType factor) -> T& { return self.template scale<ValueType> (factor); })
                .def ("scale", [](T& self, ValueType factorX, ValueType factorY) -> T& { return self.template scale<ValueType> (factorX, factorY); })
                .def ("scaled", [](const T& self, ValueType factor) { return self.template scaled<ValueType> (factor); })
                .def ("scaled", [](const T& self, ValueType factorX, ValueType factorY) { return self.template scaled<ValueType> (factorX, factorY); })
                .def ("roundToInt", [](const T& self) { return self.template roundToInt<ValueType>(); })
                .def ("toNearestInt", [](const T& self) { return self.template toNearestInt<ValueType>(); })
            ;
        }

        type[py::type::of (py::cast (Types {}))] = class_;

        return true;
    }() && ...);

    m.add_object ("Point", type);
}

// ============================================================================================

template <template <class> class Class, class... Types>
void registerLine (py::module_& m)
{
    py::dict type;

    ([&]
    {
        using ValueType = Types;
        using T = Class<ValueType>;

        const auto className = Helpers::pythonizeCompoundClassName ("Line", typeid (Types).name());

        auto class_ = py::class_<T> (m, className.toRawUTF8())
            // Constructors
            .def (py::init<>())
            .def (py::init<ValueType, ValueType, ValueType, ValueType>())
            .def (py::init<Point<ValueType>, Point<ValueType>>())
            .def (py::init<const T&>())

            // Basic methods
            .def ("getStartX", &T::getStartX)
            .def ("getStartY", &T::getStartY)
            .def ("getEndX", &T::getEndX)
            .def ("getEndY", &T::getEndY)
            .def ("getStart", &T::getStart)
            .def ("getEnd", &T::getEnd)
            .def ("setStart", &T::setStart)
            .def ("setEnd", &T::setEnd)
            .def ("withStart", &T::withStart)
            .def ("withEnd", &T::withEnd)

            // Line operations
            .def ("reverse", &T::reverse)
            .def ("reversed", &T::reversed)
            .def ("length", &T::length)
            .def ("slope", &T::slope)
            .def ("contains", py::overload_cast<const Point<ValueType>&> (&T::contains, py::const_))
            .def ("contains", py::overload_cast<const Point<ValueType>&, float> (&T::contains, py::const_))
            .def ("pointAlong", &T::pointAlong)

            // Translation
            .def ("translate", py::overload_cast<ValueType, ValueType> (&T::translate))
            .def ("translate", py::overload_cast<const Point<ValueType>&> (&T::translate))
            .def ("translated", py::overload_cast<ValueType, ValueType> (&T::translated, py::const_))
            .def ("translated", py::overload_cast<const Point<ValueType>&> (&T::translated, py::const_))

            // Extension methods
            .def ("extend", &T::extend)
            .def ("extended", &T::extended)
            .def ("extendBefore", &T::extendBefore)
            .def ("extendedBefore", &T::extendedBefore)
            .def ("extendAfter", &T::extendAfter)
            .def ("extendedAfter", &T::extendedAfter)
            .def ("keepOnlyStart", &T::keepOnlyStart)
            .def ("keepOnlyEnd", &T::keepOnlyEnd)

            // Rotation and transformation
            .def ("rotateAtPoint", &T::rotateAtPoint)
            .def ("transform", &T::transform)
            .def ("transformed", &T::transformed)

            // Conversion methods
            .def ("toInt", [](const T& self) { return self.template to<int>(); })
            .def ("toLong", [](const T& self) { return self.template to<long>(); })
            .def ("toFloat", [](const T& self) { return self.template to<float>(); })
            .def ("toDouble", [](const T& self) { return self.template to<double>(); })

            // Comparison
            .def (py::self == py::self)
            .def (py::self != py::self)

            // Operators
            .def ("__repr__", [] (const T& self)
            {
                String result;
                result
                    << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                    << "(" << self.getStartX() << ", " << self.getStartY() << ", " << self.getEndX() << ", " << self.getEndY() << ")";
                return result;
            })
            //.def ("__str__", &T::toString)
            .def ("__str__", [] (const T& self)
            {
                String result;
                result << self.getStartX() << ", " << self.getStartY() << ", " << self.getEndX() << ", " << self.getEndY();
                return result;
            })
        ;

        type[py::type::of (py::cast (Types {}))] = class_;

        return true;
    }() && ...);

    m.add_object ("Line", type);
}

// ============================================================================================

template <template <class> class Class, class... Types>
void registerSize (py::module_& m)
{
    py::dict type;

    ([&]
    {
        using ValueType = Types;
        using T = Class<ValueType>;

        const auto className = Helpers::pythonizeCompoundClassName ("Size", typeid (Types).name());

        auto class_ = py::class_<T> (m, className.toRawUTF8())
            // Constructors
            .def (py::init<>())
            .def (py::init<ValueType, ValueType>())
            .def (py::init<const T&>())

            // Width accessors
            .def ("getWidth", &T::getWidth)
            .def ("setWidth", &T::setWidth)
            .def ("withWidth", &T::withWidth)

            // Height accessors
            .def ("getHeight", &T::getHeight)
            .def ("setHeight", &T::setHeight)
            .def ("withHeight", &T::withHeight)

            // State checking methods
            .def ("isZero", &T::isZero)
            .def ("isEmpty", &T::isEmpty)
            .def ("isVerticallyEmpty", &T::isVerticallyEmpty)
            .def ("isHorizontallyEmpty", &T::isHorizontallyEmpty)
            .def ("isSquare", &T::isSquare)

            // Utility methods
            .def ("area", &T::area)
            .def ("reverse", &T::reverse)
            .def ("reversed", &T::reversed)

            // Enlarge methods
            .def ("enlarge", py::overload_cast<ValueType> (&T::enlarge))
            .def ("enlarge", py::overload_cast<ValueType, ValueType> (&T::enlarge))
            .def ("enlarged", py::overload_cast<ValueType> (&T::enlarged, py::const_))
            .def ("enlarged", py::overload_cast<ValueType, ValueType> (&T::enlarged, py::const_))

            // Reduce methods
            .def ("reduce", py::overload_cast<ValueType> (&T::reduce))
            .def ("reduce", py::overload_cast<ValueType, ValueType> (&T::reduce))
            .def ("reduced", py::overload_cast<ValueType> (&T::reduced, py::const_))
            .def ("reduced", py::overload_cast<ValueType, ValueType> (&T::reduced, py::const_))

            // Conversion methods
            .def ("toInt", [](const T& self) { return self.template to<int>(); })
            .def ("toLong", [](const T& self) { return self.template to<long>(); })
            .def ("toFloat", [](const T& self) { return self.template to<float>(); })
            .def ("toDouble", [](const T& self) { return self.template to<double>(); })
            .def ("toPoint", [](const T& self) { return self.template toPoint<ValueType>(); })
            .def ("toRectangle", [](const T& self) { return self.template toRectangle<ValueType>(); })
            .def ("toRectangle", [](const T& self, ValueType x, ValueType y) { return self.template toRectangle<ValueType> (x, y); })
            .def ("toRectangle", [](const T& self, Point<ValueType> xy) { return self.template toRectangle<ValueType> (xy); })

            // String conversion
            .def ("toString", &T::toString)

            // Comparison
            .def (py::self == py::self)
            .def (py::self != py::self)
            .def ("approximatelyEqualTo", &T::approximatelyEqualTo)

            // Operators
            .def (py::self * ValueType())
            .def (py::self *= ValueType())
            .def (py::self / ValueType())
            .def (py::self /= ValueType())

            .def ("__repr__", [](const T& self)
            {
                String result;
                result
                    << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                    << "(" << self.getWidth() << ", " << self.getHeight() << ")";
                return result;
            })
            //.def ("__str__", &T::toString)
            .def ("__str__", [](const T& self)
            {
                String result;
                result << self.getWidth() << ", " << self.getHeight();
                return result;
            })
        ;

        // Add floating-point specific methods
        if constexpr (std::is_floating_point_v<ValueType>)
        {
            class_
                .def ("scale", [](T& self, ValueType scaleFactor) -> T& { return self.template scale<ValueType> (scaleFactor); })
                .def ("scale", [](T& self, ValueType scaleFactorX, ValueType scaleFactorY) -> T& { return self.template scale<ValueType> (scaleFactorX, scaleFactorY); })
                .def ("scaled", [](const T& self, ValueType scaleFactor) { return self.template scaled<ValueType> (scaleFactor); })
                .def ("scaled", [](const T& self, ValueType scaleFactorX, ValueType scaleFactorY) { return self.template scaled<ValueType> (scaleFactorX, scaleFactorY); })
                .def ("roundToInt", [](const T& self) { return self.template roundToInt<ValueType>(); })
                .def ("toNearestInt", [](const T& self) { return self.template toNearestInt<ValueType>(); })
            ;
        }

        type[py::type::of (py::cast (Types {}))] = class_;

        return true;
    }() && ...);

    m.add_object ("Size", type);
}

// ============================================================================================

template <template <class> class Class, class... Types>
void registerRectangle (py::module_& m)
{
    py::dict type;

    ([&]
    {
        using ValueType = Types;
        using T = Class<ValueType>;

        const auto className = Helpers::pythonizeCompoundClassName ("Rectangle", typeid (Types).name());

        auto class_ = py::class_<T> (m, className.toRawUTF8())
            // Constructors
            .def (py::init<>())
            .def (py::init<ValueType, ValueType>())
            .def (py::init<ValueType, ValueType, ValueType, ValueType>())
            .def (py::init<ValueType, ValueType, const Size<ValueType>&>())
            .def (py::init<const Point<ValueType>&, ValueType, ValueType>())
            .def (py::init<const Point<ValueType>&, const Size<ValueType>&>())
            .def (py::init<const T&>())

            // Basic methods
            .def ("isEmpty", &T::isEmpty)
            .def ("isPoint", &T::isPoint)
            .def ("isLine", &T::isLine)
            .def ("isVerticalLine", &T::isVerticalLine)
            .def ("isHorizontalLine", &T::isHorizontalLine)

            // Position getters/setters
            .def ("getX", &T::getX)
            .def ("setX", &T::setX)
            .def ("withX", &T::withX)
            .def ("getY", &T::getY)
            .def ("setY", &T::setY)
            .def ("withY", &T::withY)
            .def ("getLeft", &T::getLeft)
            .def ("setLeft", &T::setLeft)
            .def ("withLeft", &T::withLeft)
            .def ("withTrimmedLeft", &T::withTrimmedLeft)
            .def ("getTop", &T::getTop)
            .def ("setTop", &T::setTop)
            .def ("withTop", &T::withTop)
            .def ("withTrimmedTop", &T::withTrimmedTop)
            .def ("getRight", &T::getRight)
            .def ("setRight", &T::setRight)
            .def ("withRight", &T::withRight)
            .def ("withTrimmedRight", &T::withTrimmedRight)
            .def ("getBottom", &T::getBottom)
            .def ("setBottom", &T::setBottom)
            .def ("withBottom", &T::withBottom)
            .def ("withTrimmedBottom", &T::withTrimmedBottom)

            // Size getters/setters
            .def ("getWidth", &T::getWidth)
            .def ("setWidth", &T::setWidth)
            .def ("withWidth", &T::withWidth)
            .def ("withWidthKeepingAspectRatio", &T::withWidthKeepingAspectRatio)
            .def ("proportionOfWidth", &T::proportionOfWidth)
            .def ("getHeight", &T::getHeight)
            .def ("setHeight", &T::setHeight)
            .def ("withHeight", &T::withHeight)
            .def ("withHeightKeepingAspectRatio", &T::withHeightKeepingAspectRatio)
            .def ("proportionOfHeight", &T::proportionOfHeight)

            // Position and size
            .def ("getPosition", &T::getPosition)
            .def ("setPosition", &T::setPosition)
            .def ("withPosition", py::overload_cast<const Point<ValueType>&> (&T::template withPosition<ValueType>, py::const_))
            .def ("withPosition", py::overload_cast<ValueType, ValueType> (&T::template withPosition<ValueType>, py::const_))
            .def ("withZeroPosition", &T::withZeroPosition)
            .def ("getSize", &T::getSize)
            .def ("setSize", py::overload_cast<const Size<ValueType>&> (&T::template setSize<ValueType>))
            .def ("setSize", py::overload_cast<ValueType, ValueType> (&T::template setSize<ValueType>))
            .def ("withSize", py::overload_cast<const Size<ValueType>&> (&T::template withSize<ValueType>, py::const_))
            .def ("withSize", py::overload_cast<ValueType, ValueType> (&T::template withSize<ValueType>, py::const_))
            .def ("withZeroSize", &T::withZeroSize)
            .def ("setBounds", &T::setBounds)

            // Corner getters/setters
            .def ("getTopLeft", &T::getTopLeft)
            .def ("setTopLeft", &T::setTopLeft)
            .def ("withTopLeft", &T::withTopLeft)
            .def ("getTopRight", &T::getTopRight)
            .def ("setTopRight", &T::setTopRight)
            .def ("withTopRight", &T::withTopRight)
            .def ("getBottomLeft", &T::getBottomLeft)
            .def ("setBottomLeft", &T::setBottomLeft)
            .def ("withBottomLeft", &T::withBottomLeft)
            .def ("getBottomRight", &T::getBottomRight)
            .def ("setBottomRight", &T::setBottomRight)
            .def ("withBottomRight", &T::withBottomRight)

            // Center methods
            .def ("getCenterX", &T::getCenterX)
            .def ("setCenterX", &T::setCenterX)
            .def ("getCenterY", &T::getCenterY)
            .def ("setCenterY", &T::setCenterY)
            .def ("getCenter", &T::getCenter)
            .def ("setCenter", py::overload_cast<ValueType, ValueType> (&T::setCenter))
            .def ("setCenter", py::overload_cast<const Point<ValueType>&> (&T::setCenter))
            .def ("withCenter", py::overload_cast<ValueType, ValueType> (&T::withCenter))
            .def ("withCenter", py::overload_cast<const Point<ValueType>&> (&T::withCenter))
            .def ("withCenterX", &T::withCenterX)
            .def ("withCenterY", &T::withCenterY)

            // Side line extraction
            .def ("leftSide", &T::leftSide)
            .def ("topSide", &T::topSide)
            .def ("rightSide", &T::rightSide)
            .def ("bottomSide", &T::bottomSide)
            .def ("diagonalTopToBottom", &T::diagonalTopToBottom)
            .def ("diagonalBottomToTop", &T::diagonalBottomToTop)

            // Translation
            .def ("translate", py::overload_cast<ValueType, ValueType> (&T::translate))
            .def ("translate", py::overload_cast<const Point<ValueType>&> (&T::translate))
            .def ("translated", py::overload_cast<ValueType, ValueType> (&T::translated, py::const_))
            .def ("translated", py::overload_cast<const Point<ValueType>&> (&T::translated, py::const_))

            // Scaling
            .def ("scale", py::overload_cast<float> (&T::scale))
            .def ("scale", py::overload_cast<float, float> (&T::scale))
            .def ("scaled", py::overload_cast<float> (&T::scaled, py::const_))
            .def ("scaled", py::overload_cast<float, float> (&T::scaled, py::const_))

            // RemoveFrom methods
            .def ("removeFromTop", &T::removeFromTop)
            .def ("removeFromLeft", &T::removeFromLeft)
            .def ("removeFromBottom", &T::removeFromBottom)
            .def ("removeFromRight", &T::removeFromRight)

            // Reduce/Enlarge methods
            .def ("reduce", py::overload_cast<ValueType> (&T::reduce))
            .def ("reduce", py::overload_cast<ValueType, ValueType> (&T::reduce))
            .def ("reduce", py::overload_cast<ValueType, ValueType, ValueType, ValueType> (&T::reduce))
            .def ("reduced", py::overload_cast<ValueType> (&T::reduced, py::const_))
            .def ("reduced", py::overload_cast<ValueType, ValueType> (&T::reduced, py::const_))
            .def ("reduced", py::overload_cast<ValueType, ValueType, ValueType, ValueType> (&T::reduced, py::const_))
            .def ("reducedLeft", &T::reducedLeft)
            .def ("reducedTop", &T::reducedTop)
            .def ("reducedRight", &T::reducedRight)
            .def ("reducedBottom", &T::reducedBottom)
            .def ("enlarge", py::overload_cast<ValueType> (&T::enlarge))
            .def ("enlarge", py::overload_cast<ValueType, ValueType> (&T::enlarge))
            .def ("enlarge", py::overload_cast<ValueType, ValueType, ValueType, ValueType> (&T::enlarge))
            .def ("enlarged", py::overload_cast<ValueType> (&T::enlarged, py::const_))
            .def ("enlarged", py::overload_cast<ValueType, ValueType> (&T::enlarged, py::const_))
            .def ("enlargedLeft", &T::enlargedLeft)
            .def ("enlargedTop", &T::enlargedTop)
            .def ("enlargedRight", &T::enlargedRight)
            .def ("enlargedBottom", &T::enlargedBottom)

            // Contains and intersection
            .def ("contains", py::overload_cast<ValueType, ValueType> (&T::contains, py::const_))
            .def ("contains", py::overload_cast<const Point<ValueType>&> (&T::contains, py::const_))
            .def ("contains", py::overload_cast<const Line<ValueType>&> (&T::contains, py::const_))
            .def ("contains", py::overload_cast<const T&> (&T::contains, py::const_))
            .def ("intersects", &T::intersects)
            .def ("intersection", &T::intersection)
            .def ("unionWith", &T::unionWith)

            // Utility methods
            .def ("area", &T::area)
            .def ("widthOverHeightRatio", &T::widthOverHeightRatio)
            .def ("heightOverWidthRatio", &T::heightOverWidthRatio)
            .def ("largestFittingSquare", &T::largestFittingSquare)
            .def ("centeredRectangleWithSize", &T::centeredRectangleWithSize)

            // Static methods
            //.def_static ("fromTwoPoints", &T::fromTwoPoints)

            // Transformation
            .def ("transformed", &T::transformed)

            // Conversion methods
            .def ("toInt", [](const T& self) { return self.template to<int>(); })
            .def ("toLong", [](const T& self) { return self.template to<long>(); })
            .def ("toFloat", [](const T& self) { return self.template to<float>(); })
            .def ("toDouble", [](const T& self) { return self.template to<double>(); })

            // Comparison
            .def (py::self == py::self)
            .def (py::self != py::self)
            .def ("approximatelyEqualTo", &T::approximatelyEqualTo)

            // Operators with Points
            //.def (py::self + Point<ValueType>())
            //.def (py::self += Point<ValueType>())
            //.def (py::self - Point<ValueType>())
            //.def (py::self -= Point<ValueType>())

            .def ("__repr__", [] (const T& self)
            {
                String result;
                result
                    << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                    << "(" << self.getX() << ", " << self.getY() << ", " << self.getWidth() << ", " << self.getHeight() << ")";
                return result;
            })
            //.def ("__str__", &T::toString)
            .def ("__str__", [](const T& self)
            {
                String result;
                result << self.getX() << ", " << self.getY() << ", " << self.getWidth() << ", " << self.getHeight();
                return result;
            })
        ;

        // Add floating-point specific methods
        if constexpr (std::is_floating_point_v<ValueType>)
        {
            class_
                .def ("withScaledSize", [](const T& self, ValueType scaleFactor) { return self.template withScaledSize<ValueType> (scaleFactor); })
                .def ("toNearestInt", [](const T& self) { return self.template toNearestInt<ValueType>(); })
                .def (py::self * ValueType())
                .def (py::self *= ValueType())
                .def (py::self / ValueType())
                .def (py::self /= ValueType())
            ;
        }

        type[py::type::of (py::cast (Types {}))] = class_;

        return true;
    }() && ...);

    m.add_object ("Rectangle", type);
}

// ============================================================================================

template <template <class> class Class, class... Types>
void registerRectangleList (py::module_& m)
{
    py::dict type;

    ([&]
    {
        using ValueType = Types;
        using T = Class<ValueType>;

        const auto className = Helpers::pythonizeCompoundClassName ("RectangleList", typeid (Types).name());

        auto class_ = py::class_<T> (m, className.toRawUTF8())
            // Constructors
            .def (py::init<>())
            .def (py::init<std::initializer_list<Rectangle<ValueType>>>())
            .def (py::init<const T&>())

            // Basic methods
            .def ("isEmpty", &T::isEmpty)
            .def ("getNumRectangles", &T::getNumRectangles)
            .def ("getRectangle", &T::getRectangle)
            .def ("getRectangles", &T::getRectangles)
            .def ("clear", &T::clear)
            .def ("clearQuick", &T::clearQuick)

            // Add and remove methods
            .def ("add", &T::add)
            .def ("addWithoutMerge", &T::addWithoutMerge)
            .def ("remove", &T::remove)

            // Contains methods
            .def ("contains", py::overload_cast<ValueType, ValueType> (&T::contains, py::const_))
            .def ("contains", py::overload_cast<const Point<ValueType>&> (&T::contains, py::const_))
            .def ("contains", py::overload_cast<ValueType, ValueType, ValueType, ValueType> (&T::contains, py::const_))
            .def ("contains", py::overload_cast<const Rectangle<ValueType>&> (&T::contains, py::const_))

            // Intersection methods
            .def ("intersects", py::overload_cast<ValueType, ValueType, ValueType, ValueType> (&T::intersects, py::const_))
            .def ("intersects", py::overload_cast<const Rectangle<ValueType>&> (&T::intersects, py::const_))

            // Bounds and utility methods
            .def ("getBoundingBox", &T::getBoundingBox)

            // Transformation methods
            .def ("offset", py::overload_cast<ValueType, ValueType> (&T::offset))
            .def ("offset", py::overload_cast<const Point<ValueType>&> (&T::offset))
            .def ("scale", py::overload_cast<float> (&T::scale))
            .def ("scale", py::overload_cast<float, float> (&T::scale))

            // Iteration support
            .def ("__iter__", [] (const T& self)
            {
                return py::make_iterator (self.begin(), self.end());
            }, py::keep_alive<0, 1>())
            .def ("__len__", &T::getNumRectangles)
            .def ("__getitem__", [](const T& self, int index)
            {
                if (index < 0 || index >= self.getNumRectangles())
                    throw py::index_error ("Rectangle index out of range");
                return self.getRectangle (index);
            })
            .def ("__bool__", [](const T& self) { return !self.isEmpty(); })

            // Representation
            .def ("__repr__", [] (const T& self)
            {
                String result;
                result
                    << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                    << "(" << self.getNumRectangles() << " rectangles)";
                return result;
            })
            //.def ("__str__", &T::toString)
        ;

        type[py::type::of (py::cast (Types {}))] = class_;

        return true;
    }() && ...);

    m.add_object ("RectangleList", type);
}

// ============================================================================================

void registerYupGraphicsBindings (py::module_& m)
{
    // ============================================================================================ yup::Justification

    py::class_<Justification> classJustification (m, "Justification");

    Helpers::makeArithmeticEnum<Justification::Flags> (classJustification, "Flags")
        .value ("left", Justification::Flags::left)
        .value ("right", Justification::Flags::right)
        .value ("horizontalCenter", Justification::Flags::horizontalCenter)
        .value ("top", Justification::Flags::top)
        .value ("bottom", Justification::Flags::bottom)
        .value ("verticalCenter", Justification::Flags::verticalCenter)
        .value ("topLeft", Justification::Flags::topLeft)
        .value ("topRight", Justification::Flags::topRight)
        .value ("bottomLeft", Justification::Flags::bottomLeft)
        .value ("bottomRight", Justification::Flags::bottomRight)
        .value ("centerLeft", Justification::Flags::centerLeft)
        .value ("centerTop", Justification::Flags::centerTop)
        .value ("center", Justification::Flags::center)
        .value ("centerRight", Justification::Flags::centerRight)
        .value ("centerBottom", Justification::Flags::centerBottom)
        .export_values();

    classJustification
        .def (py::init<Justification::Flags>())
        .def (py::init ([](int flags) { return Justification (static_cast<Justification::Flags> (flags)); }))
        .def (py::init<const Justification&>())
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def (py::self == Justification::Flags())
        .def (py::self != Justification::Flags())
        .def ("getFlags", &Justification::getFlags)
        .def ("testFlags", [](const Justification& self, Justification flags) { return self.testFlags (flags); })
        .def ("testFlags", [](const Justification& self, Justification::Flags flags) { return self.testFlags (flags); })
        .def ("withAddedFlags", &Justification::withAddedFlags)
        .def ("withRemovedFlags", &Justification::withRemovedFlags)
        //.def ("getOnlyVerticalFlags", &Justification::getOnlyVerticalFlags)
        //.def ("getOnlyHorizontalFlags", &Justification::getOnlyHorizontalFlags)
        //.def ("applyToRectangle", &Justification::template applyToRectangle<int>)
        //.def ("applyToRectangle", &Justification::template applyToRectangle<float>)
        //.def ("appliedToRectangle", &Justification::template appliedToRectangle<int>)
        //.def ("appliedToRectangle", &Justification::template appliedToRectangle<float>)
    ;

    // py::implicitly_convertible<Justification::Flags, Justification>();

    // ============================================================================================ yup::AffineTransform

    py::class_<AffineTransform> classAffineTransform (m, "AffineTransform");

    classAffineTransform
        // Constructors
        .def (py::init<>())
        .def (py::init<float, float, float, float, float, float>())
        .def (py::init<const AffineTransform&>())

        // Matrix component access
        .def ("getScaleX", &AffineTransform::getScaleX)
        .def ("getShearX", &AffineTransform::getShearX)
        .def ("getTranslateX", &AffineTransform::getTranslateX)
        .def ("getShearY", &AffineTransform::getShearY)
        .def ("getScaleY", &AffineTransform::getScaleY)
        .def ("getTranslateY", &AffineTransform::getTranslateY)
        .def ("getTranslation", &AffineTransform::getTranslation)
        .def ("getMatrixPoints", &AffineTransform::getMatrixPoints)

        // Identity and utility checks
        .def ("isIdentity", &AffineTransform::isIdentity)
        .def ("resetToIdentity", &AffineTransform::resetToIdentity)
        .def_static ("identity", &AffineTransform::identity)

        // Inversion
        .def ("inverted", &AffineTransform::inverted)

        // Point transformation
        .def ("transformPoint", [](const AffineTransform& self, int x, int y) {
            int tx = x, ty = y;
            self.transformPoint (tx, ty);
            return py::make_tuple (tx, ty);
        })
        .def ("transformPoint", [](const AffineTransform& self, float x, float y) {
            float tx = x, ty = y;
            self.transformPoint (tx, ty);
            return py::make_tuple (tx, ty);
        })
        .def ("transformPoints", [](const AffineTransform& self, int x1, int y1, int x2, int y2) {
            int tx1 = x1, ty1 = y1, tx2 = x2, ty2 = y2;
            self.transformPoints (tx1, ty1, tx2, ty2);
            return py::make_tuple (tx1, ty1, tx2, ty2);
        })
        .def ("transformPoints", [](const AffineTransform& self, float x1, float y1, float x2, float y2) {
            float tx1 = x1, ty1 = y1, tx2 = x2, ty2 = y2;
            self.transformPoints (tx1, ty1, tx2, ty2);
            return py::make_tuple (tx1, ty1, tx2, ty2);
        })

        // Translation
        .def ("translated", py::overload_cast<float, float> (&AffineTransform::translated, py::const_))
        .def ("translated", py::overload_cast<Point<float>> (&AffineTransform::translated, py::const_))
        .def_static ("translation", py::overload_cast<float, float> (&AffineTransform::translation))
        .def_static ("translation", py::overload_cast<Point<float>> (&AffineTransform::translation))
        .def ("withAbsoluteTranslation", py::overload_cast<float, float> (&AffineTransform::withAbsoluteTranslation, py::const_))
        .def ("withAbsoluteTranslation", py::overload_cast<Point<float>> (&AffineTransform::withAbsoluteTranslation, py::const_))

        // Rotation
        .def ("rotated", py::overload_cast<float> (&AffineTransform::rotated, py::const_))
        .def ("rotated", py::overload_cast<float, float, float> (&AffineTransform::rotated, py::const_))
        .def ("rotated", py::overload_cast<float, Point<float>> (&AffineTransform::rotated, py::const_))
        .def_static ("rotation", py::overload_cast<float> (&AffineTransform::rotation))
        .def_static ("rotation", py::overload_cast<float, float, float> (&AffineTransform::rotation))
        .def_static ("rotation", py::overload_cast<float, Point<float>> (&AffineTransform::rotation))

        // Scaling
        .def ("scaled", py::overload_cast<float> (&AffineTransform::scaled, py::const_))
        .def ("scaled", py::overload_cast<float, float> (&AffineTransform::scaled, py::const_))
        .def ("scaled", py::overload_cast<float, float, float, float> (&AffineTransform::scaled, py::const_))
        .def ("scaled", py::overload_cast<float, float, Point<float>> (&AffineTransform::scaled, py::const_))
        .def_static ("scaling", py::overload_cast<float> (&AffineTransform::scaling))
        .def_static ("scaling", py::overload_cast<float, float> (&AffineTransform::scaling))
        .def_static ("scaling", py::overload_cast<float, float, float, float> (&AffineTransform::scaling))
        .def_static ("scaling", py::overload_cast<float, float, Point<float>> (&AffineTransform::scaling))

        // Shearing
        .def ("sheared", &AffineTransform::sheared)
        .def_static ("shearing", py::overload_cast<float, float> (&AffineTransform::shearing))
        .def_static ("shearing", py::overload_cast<float, float, float, float> (&AffineTransform::shearing))
        .def_static ("shearing", py::overload_cast<float, float, Point<float>> (&AffineTransform::shearing))

        // Combination
        .def ("followedBy", &AffineTransform::followedBy)
        .def ("prependedBy", &AffineTransform::prependedBy)

        // Utility methods
        .def ("getDeterminant", &AffineTransform::getDeterminant)
        .def ("getScaleFactor", &AffineTransform::getScaleFactor)

        // Comparison
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def ("approximatelyEqualTo", &AffineTransform::approximatelyEqualTo)

        // Representation
        .def ("__repr__", [](const AffineTransform& self)
        {
            String repr;
            repr
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << self.getScaleX() << ", " << self.getShearX() << ", " << self.getTranslateX()
                << ", " << self.getShearY() << ", " << self.getScaleY() << ", " << self.getTranslateY() << ")";
            return repr;
        })
    ;

    // ============================================================================================ yup::Point<>

    registerPoint<Point, int, float> (m);

    // ============================================================================================ yup::Line<>

    registerLine<Line, int, float> (m);

    // ============================================================================================ yup::Size<>

    registerSize<Size, int, float> (m);

    // ============================================================================================ yup::Rectangle<>

    registerRectangle<Rectangle, int, float> (m);

    // ============================================================================================ yup::RectangleList<>

    registerRectangleList<RectangleList, int, float> (m);


    // ============================================================================================ yup::Path

    py::class_<Path> classPath (m, "Path");

    // Path::Verb enum
    py::enum_<Path::Verb> (classPath, "Verb")
        .value ("MoveTo", Path::Verb::MoveTo)
        .value ("LineTo", Path::Verb::LineTo)
        .value ("QuadTo", Path::Verb::QuadTo)
        .value ("CubicTo", Path::Verb::CubicTo)
        .value ("Close", Path::Verb::Close)
        .export_values();

    // Path::Segment struct
    py::class_<Path::Segment> (classPath, "Segment")
        .def (py::init<Path::Verb, Point<float>>())
        .def (py::init<Path::Verb, Point<float>, Point<float>>())
        .def (py::init<Path::Verb, Point<float>, Point<float>, Point<float>>())
        .def_static ("close", &Path::Segment::close)
        .def_readwrite ("verb", &Path::Segment::verb)
        .def_readwrite ("point", &Path::Segment::point)
        .def_readwrite ("controlPoint1", &Path::Segment::controlPoint1)
        .def_readwrite ("controlPoint2", &Path::Segment::controlPoint2)
        .def ("__repr__", [](const Path::Segment& self) {
            String result;
            result << "Path.Segment(" << (int)self.verb << ", " << self.point.getX() << ", " << self.point.getY() << ")";
            return result;
        });

    classPath
        // Constructors
        .def (py::init<>())
        .def (py::init<float, float>(), "x"_a, "y"_a)
        .def (py::init<const Point<float>&>(), "point"_a)
        .def (py::init<const Path&>())

        // Basic operations
        .def ("reserveSpace", &Path::reserveSpace, "numSegments"_a)
        .def ("size", &Path::size)
        .def ("isEmpty", &Path::isEmpty)
        .def ("clear", &Path::clear)

        // Path construction
        .def ("moveTo", py::overload_cast<float, float> (&Path::moveTo), "x"_a, "y"_a)
        .def ("moveTo", py::overload_cast<const Point<float>&> (&Path::moveTo), "point"_a)
        .def ("lineTo", py::overload_cast<float, float> (&Path::lineTo), "x"_a, "y"_a)
        .def ("lineTo", py::overload_cast<const Point<float>&> (&Path::lineTo), "point"_a)
        .def ("quadTo", py::overload_cast<float, float, float, float> (&Path::quadTo), "x"_a, "y"_a, "x1"_a, "y1"_a)
        .def ("quadTo", py::overload_cast<const Point<float>&, float, float> (&Path::quadTo), "controlPoint"_a, "x1"_a, "y1"_a)
        .def ("cubicTo", py::overload_cast<float, float, float, float, float, float> (&Path::cubicTo),
              "x"_a, "y"_a, "x1"_a, "y1"_a, "x2"_a, "y2"_a)
        .def ("cubicTo", py::overload_cast<const Point<float>&, float, float, float, float> (&Path::cubicTo),
              "controlPoint1"_a, "x1"_a, "y1"_a, "x2"_a, "y2"_a)
        .def ("close", &Path::close)

        // Line additions
        .def ("addLine", py::overload_cast<const Point<float>&, const Point<float>&> (&Path::addLine), "p1"_a, "p2"_a)
        .def ("addLine", py::overload_cast<const Line<float>&> (&Path::addLine), "line"_a)

        // Rectangle additions
        .def ("addRectangle", py::overload_cast<float, float, float, float> (&Path::addRectangle),
              "x"_a, "y"_a, "width"_a, "height"_a)
        .def ("addRectangle", py::overload_cast<const Rectangle<float>&> (&Path::addRectangle), "rect"_a)

        // Rounded rectangle additions
        .def ("addRoundedRectangle", py::overload_cast<float, float, float, float, float, float, float, float> (&Path::addRoundedRectangle),
              "x"_a, "y"_a, "width"_a, "height"_a, "radiusTopLeft"_a, "radiusTopRight"_a, "radiusBottomLeft"_a, "radiusBottomRight"_a)
        .def ("addRoundedRectangle", py::overload_cast<float, float, float, float, float> (&Path::addRoundedRectangle),
              "x"_a, "y"_a, "width"_a, "height"_a, "radius"_a)
        .def ("addRoundedRectangle", py::overload_cast<const Rectangle<float>&, float, float, float, float> (&Path::addRoundedRectangle),
              "rect"_a, "radiusTopLeft"_a, "radiusTopRight"_a, "radiusBottomLeft"_a, "radiusBottomRight"_a)
        .def ("addRoundedRectangle", py::overload_cast<const Rectangle<float>&, float> (&Path::addRoundedRectangle),
              "rect"_a, "radius"_a)

        // Ellipse additions
        .def ("addEllipse", py::overload_cast<float, float, float, float> (&Path::addEllipse),
              "x"_a, "y"_a, "width"_a, "height"_a)
        .def ("addEllipse", py::overload_cast<const Rectangle<float>&> (&Path::addEllipse), "rect"_a)

        // Centered ellipse additions
        .def ("addCenteredEllipse", py::overload_cast<float, float, float, float> (&Path::addCenteredEllipse),
              "centerX"_a, "centerY"_a, "radiusX"_a, "radiusY"_a)
        .def ("addCenteredEllipse", py::overload_cast<const Point<float>&, float, float> (&Path::addCenteredEllipse),
              "center"_a, "radiusX"_a, "radiusY"_a)
        .def ("addCenteredEllipse", py::overload_cast<const Point<float>&, const Size<float>&> (&Path::addCenteredEllipse),
              "center"_a, "diameter"_a)

        // Arc additions
        .def ("addArc", py::overload_cast<float, float, float, float, float, float, bool> (&Path::addArc),
              "x"_a, "y"_a, "width"_a, "height"_a, "fromRadians"_a, "toRadians"_a, "startAsNewSubPath"_a)
        .def ("addArc", py::overload_cast<const Rectangle<float>&, float, float, bool> (&Path::addArc),
              "rect"_a, "fromRadians"_a, "toRadians"_a, "startAsNewSubPath"_a)

        // Centered arc additions
        .def ("addCenteredArc", py::overload_cast<float, float, float, float, float, float, float, bool> (&Path::addCenteredArc),
              "centerX"_a, "centerY"_a, "radiusX"_a, "radiusY"_a, "rotationOfEllipse"_a, "fromRadians"_a, "toRadians"_a, "startAsNewSubPath"_a)
        .def ("addCenteredArc", py::overload_cast<const Point<float>&, float, float, float, float, float, bool> (&Path::addCenteredArc),
              "center"_a, "radiusX"_a, "radiusY"_a, "rotationOfEllipse"_a, "fromRadians"_a, "toRadians"_a, "startAsNewSubPath"_a)
        .def ("addCenteredArc", py::overload_cast<const Point<float>&, const Size<float>&, float, float, float, bool> (&Path::addCenteredArc),
              "center"_a, "diameter"_a, "rotationOfEllipse"_a, "fromRadians"_a, "toRadians"_a, "startAsNewSubPath"_a)

        // Triangle additions
        .def ("addTriangle", py::overload_cast<float, float, float, float, float, float> (&Path::addTriangle),
              "x1"_a, "y1"_a, "x2"_a, "y2"_a, "x3"_a, "y3"_a)
        .def ("addTriangle", py::overload_cast<const Point<float>&, const Point<float>&, const Point<float>&> (&Path::addTriangle),
              "p1"_a, "p2"_a, "p3"_a)

        // Polygon and star additions
        .def ("addPolygon", &Path::addPolygon, "centre"_a, "numberOfSides"_a, "radius"_a, "startAngle"_a = 0.0f)
        .def ("addStar", &Path::addStar, "centre"_a, "numberOfPoints"_a, "innerRadius"_a, "outerRadius"_a, "startAngle"_a = 0.0f)

        // Bubble addition
        .def ("addBubble", &Path::addBubble, "bodyArea"_a, "maximumArea"_a, "arrowTipPosition"_a, "cornerSize"_a, "arrowBaseWidth"_a)

        // Path operations
        .def ("createStrokePolygon", &Path::createStrokePolygon, "strokeWidth"_a)
        .def ("withRoundedCorners", &Path::withRoundedCorners, "cornerRadius"_a)
        .def ("appendPath", py::overload_cast<const Path&> (&Path::appendPath), "other"_a)
        .def ("appendPath", py::overload_cast<const Path&, const AffineTransform&> (&Path::appendPath), "other"_a, "transform"_a)
        .def ("swapWithPath", &Path::swapWithPath, "other"_a)

        // Sub-path operations
        .def ("startNewSubPath", py::overload_cast<float, float> (&Path::startNewSubPath), "x"_a, "y"_a)
        .def ("startNewSubPath", py::overload_cast<const Point<float>&> (&Path::startNewSubPath), "point"_a)
        .def ("closeSubPath", &Path::closeSubPath)
        .def ("isClosed", &Path::isClosed, "tolerance"_a = 0.001f)
        .def ("isExplicitlyClosed", &Path::isExplicitlyClosed)

        // Transformations
        .def ("transform", &Path::transform, "transform"_a)
        .def ("transformed", &Path::transformed, "transform"_a)
        .def ("scaleToFit", &Path::scaleToFit, "x"_a, "y"_a, "width"_a, "height"_a, "preserveProportions"_a)

        // Bounds and utility
        .def ("getBounds", &Path::getBounds)
        .def ("getBoundsTransformed", &Path::getBoundsTransformed, "transform"_a)
        .def ("getPointAlongPath", &Path::getPointAlongPath, "distance"_a)

        // String conversion
        .def ("toString", &Path::toString)
        .def ("fromString", &Path::fromString, "pathData"_a)

        // Comparison support
        .def (py::self == py::self)
        .def (py::self != py::self)

        // Iterator support
        .def ("__iter__", [](const Path& self) {
            return py::make_iterator (self.begin(), self.end());
        }, py::keep_alive<0, 1>())
        .def ("__len__", &Path::size)
        .def ("__bool__", [](const Path& self) { return !self.isEmpty(); })

        // Representation
        .def ("__repr__", [](const Path& self) {
            String result;
            result << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                   << "('" << self.toString() << "')";
            return result;
        })
        .def ("__str__", &Path::toString)
    ;

    // ============================================================================================ yup::Color

    py::class_<Color> (m, "Color")
        // Constructors
        .def (py::init<>())
        .def (py::init<uint32>(), "argb"_a.noconvert())
        .def (py::init<uint8, uint8, uint8>(), "red"_a.noconvert(), "green"_a.noconvert(), "blue"_a.noconvert())
        .def (py::init<uint8, uint8, uint8, uint8>(), "alpha"_a.noconvert(), "red"_a.noconvert(), "green"_a.noconvert(), "blue"_a.noconvert())
        .def (py::init<const Color&>())

        // Static factory methods
        .def_static ("fromHSV", &Color::fromHSV)
        .def_static ("fromHSL", &Color::fromHSL)
        .def_static ("fromString", &Color::fromString)
        .def_static ("fromRGB", &Color::fromRGB)
        .def_static ("fromRGBA", [] (uint8 r, uint8 g, uint8 b, uint8 a) { return Color::fromRGBA (r, g, b, a); })
        .def_static ("fromPackedRGBA", [] (uint32 colorRGBA) { return Color::fromRGBA (colorRGBA); })
        .def_static ("fromARGB", &Color::fromARGB)
        .def_static ("fromBGRA", [] (uint8 b, uint8 g, uint8 r, uint8 a) { return Color::fromBGRA (b, g, r, a); })
        .def_static ("fromPackedBGRA", [] (uint32 colorBGRA) { return Color::fromBGRA (colorBGRA); })
        .def_static ("opaqueRandom", &Color::opaqueRandom)

        // Color data access
        .def ("getARGB", &Color::getARGB)
        .def ("getRGBA", &Color::getRGBA)
        .def ("getBGRA", &Color::getBGRA)
        // .def (int() (py::self))  // implicit conversion to uint32

        // Transparency checks
        .def ("isTransparent", &Color::isTransparent)
        .def ("isSemiTransparent", &Color::isSemiTransparent)
        .def ("isOpaque", &Color::isOpaque)

        // Alpha component
        .def ("getAlpha", &Color::getAlpha)
        .def ("getAlphaFloat", &Color::getAlphaFloat)
        .def ("setAlpha", py::overload_cast<uint8> (&Color::setAlpha))
        .def ("setAlpha", py::overload_cast<float> (&Color::setAlpha))
        .def ("withAlpha", py::overload_cast<uint8> (&Color::withAlpha, py::const_))
        .def ("withAlpha", py::overload_cast<float> (&Color::withAlpha, py::const_))
        .def ("withMultipliedAlpha", py::overload_cast<uint8> (&Color::withMultipliedAlpha, py::const_))
        .def ("withMultipliedAlpha", py::overload_cast<float> (&Color::withMultipliedAlpha, py::const_))

        // Red component
        .def ("getRed", &Color::getRed)
        .def ("getRedFloat", &Color::getRedFloat)
        .def ("setRed", py::overload_cast<uint8> (&Color::setRed))
        .def ("setRed", py::overload_cast<float> (&Color::setRed))
        .def ("withRed", py::overload_cast<uint8> (&Color::withRed, py::const_))
        .def ("withRed", py::overload_cast<float> (&Color::withRed, py::const_))

        // Green component
        .def ("getGreen", &Color::getGreen)
        .def ("getGreenFloat", &Color::getGreenFloat)
        .def ("setGreen", py::overload_cast<uint8> (&Color::setGreen))
        .def ("setGreen", py::overload_cast<float> (&Color::setGreen))
        .def ("withGreen", py::overload_cast<uint8> (&Color::withGreen, py::const_))
        .def ("withGreen", py::overload_cast<float> (&Color::withGreen, py::const_))

        // Blue component
        .def ("getBlue", &Color::getBlue)
        .def ("getBlueFloat", &Color::getBlueFloat)
        .def ("setBlue", py::overload_cast<uint8> (&Color::setBlue))
        .def ("setBlue", py::overload_cast<float> (&Color::setBlue))
        .def ("withBlue", py::overload_cast<uint8> (&Color::withBlue, py::const_))
        .def ("withBlue", py::overload_cast<float> (&Color::withBlue, py::const_))

        // HSL color space
        .def ("getHue", &Color::getHue)
        .def ("getSaturation", &Color::getSaturation)
        .def ("getLuminance", &Color::getLuminance)

        // Color manipulation
        .def ("brighter", &Color::brighter, "amount"_a = 0.4f)
        .def ("darker", &Color::darker, "amount"_a = 0.4f)
        .def ("contrasting", py::overload_cast<> (&Color::contrasting, py::const_))
        .def ("contrasting", py::overload_cast<float> (&Color::contrasting, py::const_))

        // Color inversion
        .def ("invert", &Color::invert)
        .def ("inverted", &Color::inverted)
        .def ("invertAlpha", &Color::invertAlpha)
        .def ("invertedAlpha", &Color::invertedAlpha)

        // String conversion
        .def ("toString", &Color::toString)
        .def ("toStringRGB", &Color::toStringRGB, "withAlpha"_a = true)

        // Comparison
        .def (py::self == py::self)
        .def (py::self != py::self)

        // Representation
        .def ("__repr__", [] (const Color& self)
        {
            String repr;
            repr
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << self.getAlpha() << ", " << self.getRed() << ", " << self.getGreen() << ", " << self.getBlue() << ")";
            return repr;
        })
        .def ("__str__", &Color::toString)
    ;

    // ============================================================================================ yup::ColorGradient

    py::class_<ColorGradient> classColorGradient (m, "ColorGradient");

    py::enum_<ColorGradient::Type> (classColorGradient, "Type")
        .value ("Linear", ColorGradient::Type::Linear)
        .value ("Radial", ColorGradient::Type::Radial)
        .export_values();

    py::enum_<ColorGradient::Spread> (classColorGradient, "Spread")
        .value ("Pad", ColorGradient::Spread::Pad)
        .value ("Repeat", ColorGradient::Spread::Repeat)
        .value ("Reflect", ColorGradient::Spread::Reflect);

    py::class_<ColorGradient::ColorStop> (classColorGradient, "ColorStop")
        .def (py::init<>())
        .def (py::init<Color, float, float, float>(), "color"_a, "x"_a, "y"_a, "delta"_a)
        .def (py::init<Color, const Point<float>&, float>(), "color"_a, "point"_a, "delta"_a)
        .def_readwrite ("color", &ColorGradient::ColorStop::color)
        .def_readwrite ("x", &ColorGradient::ColorStop::x)
        .def_readwrite ("y", &ColorGradient::ColorStop::y)
        .def_readwrite ("delta", &ColorGradient::ColorStop::delta);

    classColorGradient
        .def (py::init<>())
        .def (py::init<Color, float, float, Color, float, float, ColorGradient::Type>(),
              "color1"_a, "x1"_a, "y1"_a, "color2"_a, "x2"_a, "y2"_a, "type"_a = ColorGradient::Type::Linear)
        .def (py::init<Color, const Point<float>&, Color, const Point<float>&, ColorGradient::Type>(),
              "color1"_a, "point1"_a, "color2"_a, "point2"_a, "type"_a = ColorGradient::Type::Linear)
        // Converted by value: build the whole stop list up front, since mutating the
        // list handed back by getStops() does not write through.
        .def (py::init<ColorGradient::Type, std::vector<ColorGradient::ColorStop>>(), "type"_a, "colorStops"_a)
        .def (py::init<const ColorGradient&>())
        .def ("getType", &ColorGradient::getType)
        .def ("getSpread", &ColorGradient::getSpread)
        .def ("withSpread", &ColorGradient::withSpread, "newSpread"_a)
        .def ("getStartColor", &ColorGradient::getStartColor)
        .def ("getStartX", &ColorGradient::getStartX)
        .def ("getStartY", &ColorGradient::getStartY)
        .def ("getStartDelta", &ColorGradient::getStartDelta)
        .def ("getFinishColor", &ColorGradient::getFinishColor)
        .def ("getFinishX", &ColorGradient::getFinishX)
        .def ("getFinishY", &ColorGradient::getFinishY)
        .def ("getFinishDelta", &ColorGradient::getFinishDelta)
        .def ("getNumStops", &ColorGradient::getNumStops)
        .def ("getStop", &ColorGradient::getStop, "index"_a)
        .def ("getStops", [] (const ColorGradient& self)
        {
            const auto stops = self.getStops();
            return std::vector<ColorGradient::ColorStop> (stops.begin(), stops.end());
        })
        .def ("getColorAt", py::overload_cast<float> (&ColorGradient::getColorAt, py::const_), "t"_a)
        .def ("getColorAt", py::overload_cast<float, float> (&ColorGradient::getColorAt, py::const_), "x"_a, "y"_a)
        .def ("getColorAt", py::overload_cast<const Point<float>&> (&ColorGradient::getColorAt, py::const_), "point"_a)
        .def ("addColorStop", py::overload_cast<Color, float, float, float> (&ColorGradient::addColorStop),
              "color"_a, "x"_a, "y"_a, "delta"_a)
        .def ("addColorStop", py::overload_cast<Color, const Point<float>&, float> (&ColorGradient::addColorStop),
              "color"_a, "point"_a, "delta"_a)
        .def ("addColorStop", py::overload_cast<Color, float> (&ColorGradient::addColorStop), "color"_a, "delta"_a)
        .def ("clearStops", &ColorGradient::clearStops)
        .def ("getRadius", &ColorGradient::getRadius)
        .def ("setAlpha", py::overload_cast<uint8> (&ColorGradient::setAlpha), "alpha"_a)
        .def ("setAlpha", py::overload_cast<float> (&ColorGradient::setAlpha), "alpha"_a)
        .def ("withAlpha", py::overload_cast<uint8> (&ColorGradient::withAlpha, py::const_), "alpha"_a)
        .def ("withAlpha", py::overload_cast<float> (&ColorGradient::withAlpha, py::const_), "alpha"_a)
        .def ("withMultipliedAlpha", py::overload_cast<uint8> (&ColorGradient::withMultipliedAlpha, py::const_), "alpha"_a)
        .def ("withMultipliedAlpha", py::overload_cast<float> (&ColorGradient::withMultipliedAlpha, py::const_), "alpha"_a)
        .def ("__repr__", [] (const ColorGradient& self)
        {
            String result;
            result
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << (self.getType() == ColorGradient::Type::Radial ? "Radial" : "Linear")
                << ", " << (int) self.getNumStops() << " stops)";
            return result;
        });

    // ============================================================================================ yup::PixelFormat

    py::enum_<PixelFormat> (m, "PixelFormat")
        .value ("Grayscale", PixelFormat::Grayscale)
        .value ("RGB", PixelFormat::RGB)
        .value ("RGBA", PixelFormat::RGBA)
        .export_values();

    // ============================================================================================ yup::ImageFormat

    py::class_<ImageFormat, PyImageFormat, py::smart_holder> classImageFormat (m, "ImageFormat");

    py::enum_<ImageFormat::Mode> (classImageFormat, "Mode")
        .value ("forReading", ImageFormat::Mode::forReading)
        .value ("forWriting", ImageFormat::Mode::forWriting)
        .export_values();

    py::class_<ImageFormat::Options> (classImageFormat, "Options")
        .def (py::init<>())
        .def ("withMetadata", &ImageFormat::Options::withMetadata, "parseMetadata"_a)
        .def ("withRawChunks", &ImageFormat::Options::withRawChunks, "parseRawChunks"_a)
        .def_readwrite ("parseMetadata", &ImageFormat::Options::parseMetadata)
        .def_readwrite ("parseRawChunks", &ImageFormat::Options::parseRawChunks)
        .def ("__repr__", [] (const ImageFormat::Options& self)
        {
            String result;
            result << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                   << "(" << (self.parseMetadata ? "metadata" : "no-metadata")
                   << ", " << (self.parseRawChunks ? "raw-chunks" : "no-raw-chunks") << ")";
            return result;
        });

    classImageFormat
        // Constructing a Python-visible ImageFormat creates the PyImageFormat
        // trampoline, so Python subclasses can be instantiated and registered.
        .def (py::init ([] ()
        {
            return std::unique_ptr<ImageFormat> (new PyImageFormat());
        }))
        .def ("getFormatName", &ImageFormat::getFormatName)
        .def ("getFileExtensions", &ImageFormat::getFileExtensions, "mode"_a)
        .def ("canHandleFile", &ImageFormat::canHandleFile, "file"_a, "mode"_a)
        .def ("canHandleStream", &ImageFormat::canHandleStream, "stream"_a, "mode"_a)
        .def ("getPossiblePixelFormats", [] (const ImageFormat& self)
        {
            py::list result;
            for (const auto format : self.getPossiblePixelFormats())
                result.append (py::cast (format));
            return result;
        })
        .def ("isCompressed", &ImageFormat::isCompressed)
        .def ("getQualityOptions", &ImageFormat::getQualityOptions)
    ;

    // ============================================================================================ yup::ImageFormatReader

    py::class_<ImageFormatReader, PyImageFormatReader, py::smart_holder> classImageFormatReader (m, "ImageFormatReader");

    classImageFormatReader
        // Python-implemented readers are constructed from raw data bytes. The
        // bytes are copied into an internal stream, so ownership never moves
        // between Python and C++.
        .def (py::init ([] (py::buffer data, const String& formatName)
        {
            auto info = data.request();
            auto* stream = new MemoryInputStream (info.ptr, info.size, true);
            return std::unique_ptr<ImageFormatReader> (new PyImageFormatReader (stream, formatName));
        }), "data"_a, "formatName"_a)
        .def_static ("readAllBytes", [] (InputStream& stream) -> py::bytes
        {
            MemoryBlock block;
            stream.readIntoMemoryBlock (block);
            return py::bytes (static_cast<const char*> (block.getData()), block.getSize());
        }, "stream"_a)
        .def ("getSourceBytes", [] (PyImageFormatReader& self) -> py::bytes
        {
            const auto bytes = self.readSourceBytes();
            return py::bytes (bytes.data(), bytes.size());
        })
        .def ("getFormatName", &ImageFormatReader::getFormatName)
        .def ("getOptions", &ImageFormatReader::getOptions, py::return_value_policy::reference_internal)
        .def ("readImage", &ImageFormatReader::readImage)
        .def ("readFrame", py::overload_cast<int> (&ImageFormatReader::readFrame), "frameIndex"_a)
        .def ("isAnimated", &ImageFormatReader::isAnimated)
        .def ("getFrameCount", &ImageFormatReader::getFrameCount)
        .def ("getLoopCount", &ImageFormatReader::getLoopCount)
        .def ("getFrameDelayMs", &ImageFormatReader::getFrameDelayMs, "frameIndex"_a)
    ;

    // ============================================================================================ yup::ImageFormatWriter

    py::class_<ImageFormatWriter, PyImageFormatWriter, py::smart_holder> classImageFormatWriter (m, "ImageFormatWriter");

    classImageFormatWriter
        // Python-implemented writers take ownership of the destination stream.
        // The stream is usually the one handed to createWriterFor(); writers
        // constructed with the (formatName, pixelFormat) overload own an
        // internal buffer retrievable through getOutputBytes().
        .def (py::init ([] (OutputStream& stream, const String& formatName, PixelFormat pixelFormat)
        {
            return std::unique_ptr<ImageFormatWriter> (new PyImageFormatWriter (std::addressof (stream), formatName, pixelFormat));
        }), "stream"_a, "formatName"_a, "pixelFormat"_a)
        .def (py::init ([] (const String& formatName, PixelFormat pixelFormat)
        {
            return std::unique_ptr<ImageFormatWriter> (new PyImageFormatWriter (new MemoryOutputStream (256), formatName, pixelFormat));
        }), "formatName"_a, "pixelFormat"_a = PixelFormat::RGBA)
        .def ("writeRawData", [] (PyImageFormatWriter& self, py::buffer data) -> bool
        {
            auto info = data.request();
            return self.writeRawData (std::string (static_cast<const char*> (info.ptr), info.size));
        }, "data"_a)
        .def ("flushStream", [] (PyImageFormatWriter& self) { return self.flushStream(); })
        .def ("getOutputBytes", [] (PyImageFormatWriter& self) -> py::bytes
        {
            auto* stream = self.getMemoryOutputStream();
            if (stream == nullptr)
                throw py::value_error ("getOutputBytes() requires an in-memory writer (constructed without a stream)");

            return py::bytes (static_cast<const char*> (stream->getData()), stream->getDataSize());
        })
        .def ("getFormatName", &ImageFormatWriter::getFormatName)
        .def ("getPixelFormat", &ImageFormatWriter::getPixelFormat)
        .def ("writeImage", &ImageFormatWriter::writeImage, "image"_a)
        .def ("flush", &ImageFormatWriter::flush)
        .def ("supportsAnimation", &ImageFormatWriter::supportsAnimation)
        .def ("beginAnimation", &ImageFormatWriter::beginAnimation, "loopCount"_a = 0)
        .def ("writeFrame", &ImageFormatWriter::writeFrame, "frame"_a, "delayMs"_a)
        .def ("endAnimation", &ImageFormatWriter::endAnimation)
    ;

#if YUP_IMAGE_FORMAT_BMP
    // ============================================================================================ yup::BmpImageFormat

    py::class_<BmpImageFormat, ImageFormat, py::smart_holder> (m, "BmpImageFormat")
        .def (py::init<>());
#endif

#if YUP_IMAGE_FORMAT_PPM
    // ============================================================================================ yup::PpmImageFormat

    py::class_<PpmImageFormat, ImageFormat, py::smart_holder> (m, "PpmImageFormat")
        .def (py::init<>());
#endif

#if YUP_IMAGE_FORMAT_TGA
    // ============================================================================================ yup::TgaImageFormat

    py::class_<TgaImageFormat, ImageFormat, py::smart_holder> (m, "TgaImageFormat")
        .def (py::init<>());
#endif

#if YUP_IMAGE_FORMAT_PNG
    // ============================================================================================ yup::PngImageFormat

    py::class_<PngImageFormat, ImageFormat, py::smart_holder> (m, "PngImageFormat")
        .def (py::init<>());
#endif

#if YUP_IMAGE_FORMAT_JPEG
    // ============================================================================================ yup::JpegImageFormat

    py::class_<JpegImageFormat, ImageFormat, py::smart_holder> (m, "JpegImageFormat")
        .def (py::init<>());
#endif

#if YUP_IMAGE_FORMAT_WEBP
    // ============================================================================================ yup::WebPImageFormat

    py::class_<WebPImageFormat, ImageFormat, py::smart_holder> (m, "WebPImageFormat")
        .def (py::init<>());
#endif

#if YUP_IMAGE_FORMAT_GIF
    // ============================================================================================ yup::GifImageFormat

    py::class_<GifImageFormat, ImageFormat, py::smart_holder> (m, "GifImageFormat")
        .def (py::init<>());
#endif

#if YUP_IMAGE_FORMAT_TIFF
    // ============================================================================================ yup::TiffImageFormat

    py::class_<TiffImageFormat, ImageFormat, py::smart_holder> (m, "TiffImageFormat")
        .def (py::init<>());
#endif

    // ============================================================================================ yup::ImageFormatManager

    py::enum_<ImageFormatType> (m, "ImageFormatType")
        .value ("bmp", ImageFormatType::bmp)
        .value ("ppm", ImageFormatType::ppm)
        .value ("png", ImageFormatType::png)
        .value ("jpeg", ImageFormatType::jpeg)
        .value ("webp", ImageFormatType::webp)
        .value ("gif", ImageFormatType::gif)
        .value ("tga", ImageFormatType::tga)
        .value ("tiff", ImageFormatType::tiff)
        .value ("all", ImageFormatType::all)
        .export_values();

    py::class_<ImageFormatManager> (m, "ImageFormatManager")
        .def (py::init<>())
        .def ("registerDefaultFormats", &ImageFormatManager::registerDefaultFormats, "types"_a = ImageFormatType::all)
        .def ("registerFormat", [] (ImageFormatManager& self, std::unique_ptr<ImageFormat> format)
        {
            // smart_holder moves ownership out of the Python wrapper; the
            // trampoline self-life-support keeps the wrapper alive so virtual
            // dispatch keeps reaching the Python overrides.
            self.registerFormat (std::move (format));
        }, "format"_a)
        .def ("getFormatFileExtensions", &ImageFormatManager::getFormatFileExtensions)
        .def ("createReaderFor", [] (ImageFormatManager& self, const File& file)
        {
            return self.createReaderFor (file);
        }, "file"_a)
        .def ("createReaderFor", [] (ImageFormatManager& self, const File& file, const ImageFormat::Options& options)
        {
            return self.createReaderFor (file, options);
        }, "file"_a, "options"_a)
        .def ("createWriterFor", [] (ImageFormatManager& self, const File& file)
        {
            return self.createWriterFor (file);
        }, "file"_a)
        .def ("createWriterFor", [] (ImageFormatManager& self, const File& file, PixelFormat pixelFormat, const StringPairArray& metadataValues, int qualityOptionIndex)
        {
            return self.createWriterFor (file, pixelFormat, metadataValues, qualityOptionIndex);
        }, "file"_a, "pixelFormat"_a, "metadataValues"_a, "qualityOptionIndex"_a)
    ;

    // ============================================================================================ yup::ImageMetadata

    py::class_<ImageMetadata, ReferenceCountedObjectPtr<ImageMetadata>> (m, "ImageMetadata")
        .def_static ("create", &ImageMetadata::create)
        .def_readwrite ("dpiX", &ImageMetadata::dpiX)
        .def_readwrite ("dpiY", &ImageMetadata::dpiY)
        .def_readwrite ("textEntries", &ImageMetadata::textEntries)
        .def ("hasRawChunk", &ImageMetadata::hasRawChunk, "key"_a)
        .def ("getRawChunk", &ImageMetadata::getRawChunk, "key"_a, py::return_value_policy::reference_internal)
        .def ("setRawChunk", &ImageMetadata::setRawChunk, "key"_a, "data"_a)
        .def ("getOrientation", &ImageMetadata::getOrientation)
        .def ("getCreationDate", &ImageMetadata::getCreationDate)
        .def ("getCameraMake", &ImageMetadata::getCameraMake)
        .def ("getCameraModel", &ImageMetadata::getCameraModel)
        .def ("getGpsCoordinates", [] (const ImageMetadata& self)
        {
            const auto coordinates = self.getGpsCoordinates();
            return py::make_tuple (coordinates.first, coordinates.second);
        })
        .def ("getImageDescription", &ImageMetadata::getImageDescription)
        .def ("getCopyright", &ImageMetadata::getCopyright)
        .def ("getSoftware", &ImageMetadata::getSoftware)
        .def ("__repr__", [] (const ImageMetadata& self)
        {
            String result;
            result
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << (int) self.textEntries.size() << " text entries, "
                << (int) self.rawChunks.size() << " raw chunks)";
            return result;
        })
    ;

    // ============================================================================================ yup::ImagePixelData

    py::class_<ImagePixelData, ReferenceCountedObjectPtr<ImagePixelData>> (m, "ImagePixelData")
        .def ("getWidth", &ImagePixelData::getWidth)
        .def ("getHeight", &ImagePixelData::getHeight)
        .def ("getPixelFormat", &ImagePixelData::getPixelFormat)
        .def ("getPixelStride", &ImagePixelData::getPixelStride)
        .def ("getPixel", &ImagePixelData::getPixel, "x"_a, "y"_a)
        .def ("getPixelColor", &ImagePixelData::getPixelColor, "x"_a, "y"_a)
        .def ("setPixel", py::overload_cast<int, int, uint32> (&ImagePixelData::setPixel), "x"_a, "y"_a, "color"_a)
        .def ("setPixelColor", &ImagePixelData::setPixelColor, "x"_a, "y"_a, "color"_a)
        .def ("fill", &ImagePixelData::fill, "color"_a)
        .def ("fillColor", &ImagePixelData::fillColor, "color"_a)
        .def ("clear", &ImagePixelData::clear)
        .def ("getRawData", [] (const ImagePixelData& self)
        {
            const auto data = self.getRawData();
            return py::bytes (reinterpret_cast<const char*> (data.data()), data.size());
        })
        .def ("toRGBA", [] (const ImagePixelData& self, bool premultiplyAlpha)
        {
            const auto data = self.toRGBA (premultiplyAlpha);
            return py::bytes (reinterpret_cast<const char*> (data.data()), data.size());
        }, "premultiplyAlpha"_a = true)
    ;

    // ============================================================================================ yup::Image

    py::class_<Image> classImage (m, "Image");

    classImage
        .def (py::init<>())
        .def (py::init<int, int, PixelFormat>(), "width"_a, "height"_a, "format"_a = PixelFormat::RGBA)
        .def (py::init<const Image&>())
        .def ("isValid", &Image::isValid)
        .def ("getWidth", &Image::getWidth)
        .def ("getHeight", &Image::getHeight)
        .def ("getPixelFormat", &Image::getPixelFormat)
        .def ("getPixelStride", &Image::getPixelStride)

        // Pixel access
        .def ("getPixel", &Image::getPixel, "x"_a, "y"_a)
        .def ("getPixelColor", &Image::getPixelColor, "x"_a, "y"_a)
        .def ("setPixel", &Image::setPixel, "x"_a, "y"_a, "color"_a)
        .def ("setPixelColor", &Image::setPixelColor, "x"_a, "y"_a, "color"_a)
        .def ("fill", &Image::fill, "color"_a)
        .def ("fillColor", &Image::fillColor, "color"_a)
        .def ("clear", &Image::clear)

        // Raw access
        .def ("getPixelData", [] (Image& self) -> ImagePixelData& { return self.getPixelData(); },
              py::return_value_policy::reference_internal)
        .def ("getRawData", [] (const Image& self)
        {
            const auto data = self.getRawData();
            return py::bytes (reinterpret_cast<const char*> (data.data()), data.size());
        })

        // Copying and metadata
        .def ("duplicate", &Image::duplicate)
        .def ("hasMetadata", &Image::hasMetadata)
        .def ("getMetadata", &Image::getMetadata, py::return_value_policy::reference_internal)
        .def ("setMetadata", &Image::setMetadata, "metadata"_a)

        // Loading
        .def_static ("loadFromData", [] (py::buffer data) -> Image
        {
            auto info = data.request();
            auto result = Image::loadFromData (Span<const uint8> (static_cast<const uint8*> (info.ptr), info.size));
            if (! result.wasOk())
                throw py::value_error (std::string (result.getErrorMessage().toRawUTF8()));

            return result.getValue();
        }, "data"_a)
        .def_static ("loadFromData", [] (py::buffer data, const ImageFormat::Options& options) -> Image
        {
            auto info = data.request();
            auto result = Image::loadFromData (Span<const uint8> (static_cast<const uint8*> (info.ptr), info.size), options);
            if (! result.wasOk())
                throw py::value_error (std::string (result.getErrorMessage().toRawUTF8()));

            return result.getValue();
        }, "data"_a, "options"_a)

        // GPU integration
        .def_static ("fromTexture", &Image::fromTexture, "texture"_a)
        .def_static ("fromTarget", &Image::fromTarget, "target"_a)
        .def ("getGpuTexture", &Image::getGpuTexture)
        .def ("setGpuTexture", &Image::setGpuTexture, "texture"_a)

        // Representation
        .def ("__repr__", [] (const Image& self)
        {
            String formatName;
            if (self.isValid())
            {
                switch (self.getPixelFormat())
                {
                    case PixelFormat::Grayscale: formatName = "Grayscale"; break;
                    case PixelFormat::RGB: formatName = "RGB"; break;
                    case PixelFormat::RGBA: formatName = "RGBA"; break;
                }
            }

            String result;
            result
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << (self.isValid() ? "valid" : "null") << ", "
                << self.getWidth() << "x" << self.getHeight() << ", "
                << formatName << ")";
            return result;
        })
    ;

    // ============================================================================================ yup::Font

    py::class_<Font> classFont (m, "Font");

    py::class_<Font::Axis> (classFont, "Axis")
        .def (py::init<>())
        .def_readwrite ("tagName", &Font::Axis::tagName)
        .def_readwrite ("minimumValue", &Font::Axis::minimumValue)
        .def_readwrite ("maximumValue", &Font::Axis::maximumValue)
        .def_readwrite ("defaultValue", &Font::Axis::defaultValue)
        .def ("__repr__", [] (const Font::Axis& self)
        {
            String result;
            result << "Font.Axis('" << self.tagName << "', " << self.minimumValue << ", " << self.maximumValue << ")";
            return result;
        });

    py::class_<Font::AxisOption> (classFont, "AxisOption")
        .def (py::init<const String&, float>(), "tagName"_a, "value"_a)
        .def_readwrite ("tagName", &Font::AxisOption::tagName)
        .def_readwrite ("value", &Font::AxisOption::value);

    py::class_<Font::Feature> (classFont, "Feature")
        .def (py::init<uint32, uint32>(), "tag"_a, "value"_a)
        .def (py::init ([] (const String& stringTag, uint32 value) -> Font::Feature
        {
            if (stringTag.length() != 4)
                throw py::value_error ("Feature tag must be exactly 4 characters long");

            const uint32 tag = (uint32 (stringTag[0]) << 24) | (uint32 (stringTag[1]) << 16)
                             | (uint32 (stringTag[2]) << 8) | uint32 (stringTag[3]);
            return { tag, value };
        }), "tag"_a, "value"_a)
        .def_readwrite ("tag", &Font::Feature::tag)
        .def_readwrite ("value", &Font::Feature::value);

    classFont
        // Constructors
        .def (py::init<>())
        .def (py::init<const Font&>())

        // Loading fonts
        .def_static ("loadFontFromData", [] (py::buffer data) -> Font
        {
            auto info = data.request();
            auto result = Font::loadFontFromData (Span<const uint8> (static_cast<const uint8*> (info.ptr), info.size));
            if (! result.wasOk())
                throw py::value_error (std::string (result.getErrorMessage().toRawUTF8()));

            return result.getValue();
        }, "data"_a)
        .def_static ("loadFontFromFile", [] (const File& file) -> Font
        {
            auto result = Font::loadFontFromFile (file);
            if (! result.wasOk())
                throw py::value_error (std::string (result.getErrorMessage().toRawUTF8()));

            return result.getValue();
        }, "file"_a)
        .def_static ("loadFontFromFirstAvailableFile", [] (const std::vector<std::string>& fontPaths) -> Font
        {
            for (const auto& path : fontPaths)
            {
                auto result = Font::loadFontFromFile (File (path.c_str()));
                if (result.wasOk())
                    return result.getValue();
            }

            throw py::value_error ("None of the provided font files could be loaded");
        }, "fontPaths"_a)
        .def_static ("loadSerifSystemTextFont", [] () -> Font
        {
            auto result = Font::loadSerifSystemTextFont();
            if (! result.wasOk())
                throw py::value_error (std::string (result.getErrorMessage().toRawUTF8()));

            return result.getValue();
        })
        .def_static ("loadMonospaceSystemTextFont", [] () -> Font
        {
            auto result = Font::loadMonospaceSystemTextFont();
            if (! result.wasOk())
                throw py::value_error (std::string (result.getErrorMessage().toRawUTF8()));

            return result.getValue();
        })

        // Metrics
        .def ("isEmpty", &Font::isEmpty)
        .def ("getAscent", &Font::getAscent)
        .def ("getDescent", &Font::getDescent)
        .def ("getWeight", &Font::getWeight)
        .def ("isItalic", &Font::isItalic)
        .def ("getHeight", &Font::getHeight)
        .def ("setHeight", &Font::setHeight, "newHeight"_a)
        .def ("withHeight", &Font::withHeight, "height"_a)

        // Variable font axis
        .def ("getNumAxis", &Font::getNumAxis)
        .def ("getAxisDescription", [] (const Font& self, int index) -> py::object
        {
            const auto description = self.getAxisDescription (index);
            if (! description.has_value())
                return py::none();

            return py::cast (*description);
        }, "index"_a)
        .def ("getAxisValue", py::overload_cast<int> (&Font::getAxisValue, py::const_), "index"_a)
        .def ("setAxisValue", py::overload_cast<int, float> (&Font::setAxisValue), "index"_a, "value"_a)
        .def ("withAxisValue", py::overload_cast<int, float> (&Font::withAxisValue, py::const_), "index"_a, "value"_a)
        .def ("resetAxisValue", py::overload_cast<int> (&Font::resetAxisValue), "index"_a)
        .def ("getAxisValue", [] (const Font& self, const String& tagName) { return self.getAxisValue (StringRef (tagName)); }, "tagName"_a)
        .def ("setAxisValue", [] (Font& self, const String& tagName, float value) { self.setAxisValue (StringRef (tagName), value); }, "tagName"_a, "value"_a)
        .def ("withAxisValue", [] (const Font& self, const String& tagName, float value) { return self.withAxisValue (StringRef (tagName), value); }, "tagName"_a, "value"_a)
        .def ("resetAxisValue", [] (Font& self, const String& tagName) { self.resetAxisValue (StringRef (tagName)); }, "tagName"_a)
        .def ("resetAllAxisValues", &Font::resetAllAxisValues)
        .def ("setAxisValues", [] (Font& self, const std::vector<Font::AxisOption>& axisOptions)
        {
            for (const auto& option : axisOptions)
                self.setAxisValue (StringRef (option.tagName), option.value);
        }, "axisOptions"_a)
        .def ("withAxisValues", [] (const Font& self, const std::vector<Font::AxisOption>& axisOptions)
        {
            Font result = self;
            for (const auto& option : axisOptions)
                result.setAxisValue (StringRef (option.tagName), option.value);

            return result;
        }, "axisOptions"_a)

        // OpenType features
        .def ("withFeature", &Font::withFeature, "feature"_a)
        .def ("withFeatures", [] (const Font& self, const std::vector<Font::Feature>& features)
        {
            Font result = self;
            for (const auto& feature : features)
                result = result.withFeature (feature);

            return result;
        }, "features"_a)

        // Comparison
        .def (py::self == py::self)
        .def (py::self != py::self)

        // Representation
        .def ("__repr__", [] (const Font& self)
        {
            String result;
            result
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << self.getHeight() << ", weight " << self.getWeight() << ")";
            return result;
        })
    ;

    // ============================================================================================ yup::StyledText

    py::class_<StyledText> classStyledText (m, "StyledText");

    py::enum_<StyledText::HorizontalAlign> (classStyledText, "HorizontalAlign")
        .value ("left", StyledText::HorizontalAlign::left)
        .value ("center", StyledText::HorizontalAlign::center)
        .value ("right", StyledText::HorizontalAlign::right)
        .value ("justified", StyledText::HorizontalAlign::justified);

    py::enum_<StyledText::VerticalAlign> (classStyledText, "VerticalAlign")
        .value ("top", StyledText::VerticalAlign::top)
        .value ("middle", StyledText::VerticalAlign::middle)
        .value ("bottom", StyledText::VerticalAlign::bottom);

    py::enum_<StyledText::TextOverflow> (classStyledText, "TextOverflow")
        .value ("visible", StyledText::TextOverflow::visible)
        .value ("ellipsis", StyledText::TextOverflow::ellipsis);

    py::enum_<StyledText::TextOrigin> (classStyledText, "TextOrigin")
        .value ("topOrigin", StyledText::TextOrigin::topOrigin)
        .value ("baseline", StyledText::TextOrigin::baseline);

    py::enum_<StyledText::TextWrap> (classStyledText, "TextWrap")
        .value ("wrap", StyledText::TextWrap::wrap)
        .value ("noWrap", StyledText::TextWrap::noWrap);

    py::class_<StyledText::TextModifier> (classStyledText, "TextModifier")
        .def (py::init<StyledText&>(), "styledText"_a, py::keep_alive<1, 2>())
        .def ("clear", &StyledText::TextModifier::clear)
        .def ("appendText", [] (StyledText::TextModifier& self, const String& text, const Font& font, float lineHeight, float letterSpacing)
        {
            self.appendText (text, font, lineHeight, letterSpacing);
        }, "text"_a, "font"_a, "lineHeight"_a = -1.0f, "letterSpacing"_a = 0.0f)
        .def ("appendText", [] (StyledText::TextModifier& self, const String& text, Color color, const Font& font, float lineHeight, float letterSpacing)
        {
            self.appendText (text, color, font, lineHeight, letterSpacing);
        }, "text"_a, "color"_a, "font"_a, "lineHeight"_a = -1.0f, "letterSpacing"_a = 0.0f)
        .def ("setOverflow", &StyledText::TextModifier::setOverflow, "value"_a)
        .def ("setHorizontalAlign", &StyledText::TextModifier::setHorizontalAlign, "value"_a)
        .def ("setVerticalAlign", &StyledText::TextModifier::setVerticalAlign, "value"_a)
        .def ("setMaxSize", &StyledText::TextModifier::setMaxSize, "value"_a)
        .def ("setParagraphSpacing", &StyledText::TextModifier::setParagraphSpacing, "value"_a)
        .def ("setWrap", &StyledText::TextModifier::setWrap, "value"_a)
    ;

    classStyledText
        .def (py::init<>())
        .def ("isEmpty", &StyledText::isEmpty)
        .def ("needsUpdate", &StyledText::needsUpdate)
        .def ("startUpdate", &StyledText::startUpdate)

        .def ("getOverflow", &StyledText::getOverflow)
        .def ("getHorizontalAlign", &StyledText::getHorizontalAlign)
        .def ("getVerticalAlign", &StyledText::getVerticalAlign)
        .def ("getMaxSize", &StyledText::getMaxSize)
        .def ("getParagraphSpacing", &StyledText::getParagraphSpacing)
        .def ("getWrap", &StyledText::getWrap)
        .def ("getComputedTextBounds", &StyledText::getComputedTextBounds)
        .def ("getOffset", &StyledText::getOffset, "area"_a)

        .def ("getGlyphIndexAtPosition", &StyledText::getGlyphIndexAtPosition, "position"_a)
        .def ("getCaretBounds", &StyledText::getCaretBounds, "characterIndex"_a)
        .def ("getGlyphIndexOnAdjacentLine", &StyledText::getGlyphIndexOnAdjacentLine, "characterIndex"_a, "moveDown"_a)
        .def ("getSelectionRectangles", &StyledText::getSelectionRectangles, "startIndex"_a, "endIndex"_a)
        .def ("isValidCharacterIndex", &StyledText::isValidCharacterIndex, "characterIndex"_a)

        .def_static ("horizontalAlignFromJustification", &StyledText::horizontalAlignFromJustification)
        .def_static ("verticalAlignFromJustification", &StyledText::verticalAlignFromJustification)

        .def ("__repr__", [] (const StyledText& self)
        {
            String result;
            result
                << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name())
                << "(" << (self.isEmpty() ? "empty" : "not-empty") << ")";
            return result;
        })
    ;

    // ============================================================================================ yup::GraphicsContext

    py::class_<GraphicsContext, std::unique_ptr<GraphicsContext, py::nodelete>> (m, "GraphicsContext")
        .def ("isGpuAvailable", &GraphicsContext::isGpuAvailable)
        .def ("getPlatform", &GraphicsContext::getPlatform)
        .def ("getGpuDevice", &GraphicsContext::getGpuDevice)
        .def ("tick", &GraphicsContext::tick)
    ;

    // ============================================================================================ yup::BlendMode

    py::enum_<BlendMode> (m, "BlendMode")
        .value ("SrcOver", BlendMode::SrcOver)
        .value ("Screen", BlendMode::Screen)
        .value ("Overlay", BlendMode::Overlay)
        .value ("Darken", BlendMode::Darken)
        .value ("Lighten", BlendMode::Lighten)
        .value ("ColorDodge", BlendMode::ColorDodge)
        .value ("ColorBurn", BlendMode::ColorBurn)
        .value ("HardLight", BlendMode::HardLight)
        .value ("SoftLight", BlendMode::SoftLight)
        .value ("Difference", BlendMode::Difference)
        .value ("Exclusion", BlendMode::Exclusion)
        .value ("Multiply", BlendMode::Multiply)
        .value ("Hue", BlendMode::Hue)
        .value ("Saturation", BlendMode::Saturation)
        .value ("Color", BlendMode::Color)
        .value ("Luminosity", BlendMode::Luminosity);

    // ============================================================================================ yup::StrokeCap

    py::enum_<StrokeCap> (m, "StrokeCap")
        .value ("Butt", StrokeCap::Butt)
        .value ("Round", StrokeCap::Round)
        .value ("Square", StrokeCap::Square);

    // ============================================================================================ yup::StrokeJoin

    py::enum_<StrokeJoin> (m, "StrokeJoin")
        .value ("Miter", StrokeJoin::Miter)
        .value ("Round", StrokeJoin::Round)
        .value ("Bevel", StrokeJoin::Bevel);

    // ============================================================================================ yup::Graphics

    py::class_<StrokeType> classStrokeType (m, "StrokeType");
    classStrokeType
        .def (py::init<>())
        .def (py::init<float>())
        .def (py::init<float, StrokeCap>())
        .def (py::init<float, StrokeJoin>())
        .def (py::init<float, StrokeJoin, StrokeCap>())
        .def (py::init<const StrokeType&>())
        .def ("getWidth", &StrokeType::getWidth)
        .def ("withWidth", &StrokeType::withWidth)
        .def ("getCap", &StrokeType::getCap)
        .def ("withCap", &StrokeType::withCap)
        .def ("getJoin", &StrokeType::getJoin)
        .def ("withJoin", &StrokeType::withJoin)
        .def (py::self == py::self)
        .def (py::self != py::self)
    ;

    // ============================================================================================ yup::Graphics

    py::class_<Graphics> classGraphics (m, "Graphics");

    struct PyGraphicsSaveState
    {
        PyGraphicsSaveState (Graphics& g)
            : g (g)
        {
        }

        Graphics& g;
        std::variant<std::monostate, Graphics::SavedState> state;
    };

    py::class_<PyGraphicsSaveState> (classGraphics, "SavedState")
        .def (py::init<Graphics&>())
        .def ("__enter__", [] (PyGraphicsSaveState& self)
        {
            self.state.emplace<Graphics::SavedState> (self.g.saveState());
            return std::addressof (self);
        })
        .def ("__exit__", [] (PyGraphicsSaveState& self, const std::optional<py::type>&, const std::optional<py::object>&, const std::optional<py::object>&)
        {
            self.state.emplace<std::monostate>();
        })
        .def ("restore", [] (PyGraphicsSaveState& self)
        {
            if (auto* savedState = std::get_if<Graphics::SavedState> (std::addressof (self.state)))
                savedState->restore();
        })
    ;

    classGraphics
        // Color and gradient properties
        .def ("setFillColor", &Graphics::setFillColor)
        .def ("getFillColor", &Graphics::getFillColor)
        .def ("setStrokeColor", &Graphics::setStrokeColor)
        .def ("getStrokeColor", &Graphics::getStrokeColor)
        .def ("setFillColorGradient", &Graphics::setFillColorGradient)
        .def ("getFillColorGradient", &Graphics::getFillColorGradient)
        .def ("setStrokeColorGradient", &Graphics::setStrokeColorGradient)
        .def ("getStrokeColorGradient", &Graphics::getStrokeColorGradient)

        // Stroke properties
        .def ("setStrokeType", &Graphics::setStrokeType)
        .def ("getStrokeType", &Graphics::getStrokeType)
        .def ("setStrokeWidth", &Graphics::setStrokeWidth)
        .def ("getStrokeWidth", &Graphics::getStrokeWidth)
        .def ("setStrokeJoin", &Graphics::setStrokeJoin)
        .def ("getStrokeJoin", &Graphics::getStrokeJoin)
        .def ("setStrokeCap", &Graphics::setStrokeCap)
        .def ("getStrokeCap", &Graphics::getStrokeCap)

        // Rendering properties
        .def ("setFeather", &Graphics::setFeather)
        .def ("getFeather", &Graphics::getFeather)
        .def ("setOpacity", &Graphics::setOpacity)
        .def ("getOpacity", &Graphics::getOpacity)
        .def ("setBlendMode", &Graphics::setBlendMode)
        .def ("getBlendMode", &Graphics::getBlendMode)

        // Drawing area and transformations
        .def ("setDrawingArea", &Graphics::setDrawingArea)
        .def ("getDrawingArea", &Graphics::getDrawingArea)
        .def ("setTransform", &Graphics::setTransform)
        .def ("getTransform", &Graphics::getTransform)

        // Clipping
        .def ("setClipPath", py::overload_cast<const Rectangle<float>&> (&Graphics::setClipPath))
        .def ("setClipPath", py::overload_cast<const Path&> (&Graphics::setClipPath))
        .def ("getClipPath", &Graphics::getClipPath)

        // Line drawing
        .def ("strokeLine", py::overload_cast<float, float, float, float> (&Graphics::strokeLine))
        .def ("strokeLine", py::overload_cast<const Point<float>&, const Point<float>&> (&Graphics::strokeLine))

        // Fill operations
        .def ("fillAll", &Graphics::fillAll)
        .def ("fillRect", py::overload_cast<float, float, float, float> (&Graphics::fillRect))
        .def ("fillRect", py::overload_cast<const Rectangle<float>&> (&Graphics::fillRect))

        // Stroke operations
        .def ("strokeRect", py::overload_cast<float, float, float, float> (&Graphics::strokeRect))
        .def ("strokeRect", py::overload_cast<const Rectangle<float>&> (&Graphics::strokeRect))

        // Rounded rectangle operations
        .def ("fillRoundedRect", py::overload_cast<float, float, float, float, float, float, float, float> (&Graphics::fillRoundedRect))
        .def ("fillRoundedRect", py::overload_cast<const Rectangle<float>&, float, float, float, float> (&Graphics::fillRoundedRect))
        .def ("fillRoundedRect", py::overload_cast<float, float, float, float, float> (&Graphics::fillRoundedRect))
        .def ("fillRoundedRect", py::overload_cast<const Rectangle<float>&, float> (&Graphics::fillRoundedRect))
        .def ("strokeRoundedRect", py::overload_cast<float, float, float, float, float, float, float, float> (&Graphics::strokeRoundedRect))
        .def ("strokeRoundedRect", py::overload_cast<const Rectangle<float>&, float, float, float, float> (&Graphics::strokeRoundedRect))
        .def ("strokeRoundedRect", py::overload_cast<float, float, float, float, float> (&Graphics::strokeRoundedRect))
        .def ("strokeRoundedRect", py::overload_cast<const Rectangle<float>&, float> (&Graphics::strokeRoundedRect))

        // Ellipse operations
        .def ("fillEllipse", py::overload_cast<const Rectangle<float>&> (&Graphics::fillEllipse))
        .def ("fillEllipse", py::overload_cast<float, float, float, float> (&Graphics::fillEllipse))
        .def ("strokeEllipse", py::overload_cast<const Rectangle<float>&> (&Graphics::strokeEllipse))
        .def ("strokeEllipse", py::overload_cast<float, float, float, float> (&Graphics::strokeEllipse))

        // Path operations
        .def ("fillPath", &Graphics::fillPath)
        .def ("strokePath", &Graphics::strokePath)

        // Image operations
        .def ("drawImageAt", &Graphics::drawImageAt)
        .def ("drawImage", &Graphics::drawImage)
        .def ("drawTexture", &Graphics::drawTexture)

        // Text operations
        .def ("fillFittedText", py::overload_cast<const String&, const Font&, const Rectangle<float>&, Justification> (&Graphics::fillFittedText))
        .def ("strokeFittedText", py::overload_cast<const String&, const Font&, const Rectangle<float>&, Justification> (&Graphics::strokeFittedText))

        // State management
        .def ("saveState", [](Graphics& self) { return PyGraphicsSaveState{self}; })

        // Utility methods
        .def ("getContextScale", &Graphics::getContextScale)
        .def ("getFactory", &Graphics::getFactory, py::return_value_policy::reference_internal)
        .def ("getRenderer", &Graphics::getRenderer, py::return_value_policy::reference_internal)
        .def ("getGraphicsContext", &Graphics::getGraphicsContext, py::return_value_policy::reference)
    ;

    // ============================================================================================ yup::GpuCanvas

    py::class_<GpuCanvas, ReferenceCountedObjectPtr<GpuCanvas>> (m, "GpuCanvas")
        .def_static ("create", &GpuCanvas::create)
        .def ("getWidth", &GpuCanvas::getWidth)
        .def ("getHeight", &GpuCanvas::getHeight)
        .def ("asTexture", &GpuCanvas::asTexture)
        .def ("asImage", &GpuCanvas::asImage)
        .def ("beginDraw", &GpuCanvas::beginDraw)
        .def ("commit", &GpuCanvas::commit)
        .def ("getTarget", &GpuCanvas::getTarget)
        .def ("__repr__", [] (const GpuCanvas& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " " << self.getWidth() << "x" << self.getHeight() << ">";
            return result;
        });

    // ============================================================================================ yup::Colors

    auto submoduleColors = m.def_submodule ("Colors");
    submoduleColors.def ("getNamedColor", &Colors::getNamedColor);
    submoduleColors.attr ("transparentBlack") = Colors::transparentBlack;
    submoduleColors.attr ("transparentWhite") = Colors::transparentWhite;
    submoduleColors.attr ("aliceblue") = Colors::aliceblue;
    submoduleColors.attr ("antiquewhite") = Colors::antiquewhite;
    submoduleColors.attr ("aqua") = Colors::aqua;
    submoduleColors.attr ("aquamarine") = Colors::aquamarine;
    submoduleColors.attr ("azure") = Colors::azure;
    submoduleColors.attr ("beige") = Colors::beige;
    submoduleColors.attr ("bisque") = Colors::bisque;
    submoduleColors.attr ("black") = Colors::black;
    submoduleColors.attr ("blanchedalmond") = Colors::blanchedalmond;
    submoduleColors.attr ("blue") = Colors::blue;
    submoduleColors.attr ("blueviolet") = Colors::blueviolet;
    submoduleColors.attr ("brown") = Colors::brown;
    submoduleColors.attr ("burlywood") = Colors::burlywood;
    submoduleColors.attr ("cadetblue") = Colors::cadetblue;
    submoduleColors.attr ("chartreuse") = Colors::chartreuse;
    submoduleColors.attr ("chocolate") = Colors::chocolate;
    submoduleColors.attr ("coral") = Colors::coral;
    submoduleColors.attr ("cornflowerblue") = Colors::cornflowerblue;
    submoduleColors.attr ("cornsilk") = Colors::cornsilk;
    submoduleColors.attr ("crimson") = Colors::crimson;
    submoduleColors.attr ("cyan") = Colors::cyan;
    submoduleColors.attr ("darkblue") = Colors::darkblue;
    submoduleColors.attr ("darkcyan") = Colors::darkcyan;
    submoduleColors.attr ("darkgoldenrod") = Colors::darkgoldenrod;
    submoduleColors.attr ("darkgray") = Colors::darkgray;
    submoduleColors.attr ("darkgreen") = Colors::darkgreen;
    submoduleColors.attr ("darkkhaki") = Colors::darkkhaki;
    submoduleColors.attr ("darkmagenta") = Colors::darkmagenta;
    submoduleColors.attr ("darkolivegreen") = Colors::darkolivegreen;
    submoduleColors.attr ("darkorange") = Colors::darkorange;
    submoduleColors.attr ("darkorchid") = Colors::darkorchid;
    submoduleColors.attr ("darkred") = Colors::darkred;
    submoduleColors.attr ("darksalmon") = Colors::darksalmon;
    submoduleColors.attr ("darkseagreen") = Colors::darkseagreen;
    submoduleColors.attr ("darkslateblue") = Colors::darkslateblue;
    submoduleColors.attr ("darkslategray") = Colors::darkslategray;
    submoduleColors.attr ("darkturquoise") = Colors::darkturquoise;
    submoduleColors.attr ("darkviolet") = Colors::darkviolet;
    submoduleColors.attr ("deeppink") = Colors::deeppink;
    submoduleColors.attr ("deepskyblue") = Colors::deepskyblue;
    submoduleColors.attr ("dimgray") = Colors::dimgray;
    submoduleColors.attr ("dodgerblue") = Colors::dodgerblue;
    submoduleColors.attr ("firebrick") = Colors::firebrick;
    submoduleColors.attr ("floralwhite") = Colors::floralwhite;
    submoduleColors.attr ("forestgreen") = Colors::forestgreen;
    submoduleColors.attr ("fuchsia") = Colors::fuchsia;
    submoduleColors.attr ("gainsboro") = Colors::gainsboro;
    submoduleColors.attr ("ghostwhite") = Colors::ghostwhite;
    submoduleColors.attr ("gold") = Colors::gold;
    submoduleColors.attr ("goldenrod") = Colors::goldenrod;
    submoduleColors.attr ("gray") = Colors::gray;
    submoduleColors.attr ("green") = Colors::green;
    submoduleColors.attr ("greenyellow") = Colors::greenyellow;
    submoduleColors.attr ("honeydew") = Colors::honeydew;
    submoduleColors.attr ("hotpink") = Colors::hotpink;
    submoduleColors.attr ("indianred") = Colors::indianred;
    submoduleColors.attr ("indigo") = Colors::indigo;
    submoduleColors.attr ("ivory") = Colors::ivory;
    submoduleColors.attr ("khaki") = Colors::khaki;
    submoduleColors.attr ("lavender") = Colors::lavender;
    submoduleColors.attr ("lavenderblush") = Colors::lavenderblush;
    submoduleColors.attr ("lawngreen") = Colors::lawngreen;
    submoduleColors.attr ("lemonchiffon") = Colors::lemonchiffon;
    submoduleColors.attr ("lightblue") = Colors::lightblue;
    submoduleColors.attr ("lightcoral") = Colors::lightcoral;
    submoduleColors.attr ("lightcyan") = Colors::lightcyan;
    submoduleColors.attr ("lightgoldenrodyellow") = Colors::lightgoldenrodyellow;
    submoduleColors.attr ("lightgreen") = Colors::lightgreen;
    submoduleColors.attr ("lightgray") = Colors::lightgray;
    submoduleColors.attr ("lightpink") = Colors::lightpink;
    submoduleColors.attr ("lightsalmon") = Colors::lightsalmon;
    submoduleColors.attr ("lightseagreen") = Colors::lightseagreen;
    submoduleColors.attr ("lightskyblue") = Colors::lightskyblue;
    submoduleColors.attr ("lightslategray") = Colors::lightslategray;
    submoduleColors.attr ("lightsteelblue") = Colors::lightsteelblue;
    submoduleColors.attr ("lightyellow") = Colors::lightyellow;
    submoduleColors.attr ("lime") = Colors::lime;
    submoduleColors.attr ("limegreen") = Colors::limegreen;
    submoduleColors.attr ("linen") = Colors::linen;
    submoduleColors.attr ("magenta") = Colors::magenta;
    submoduleColors.attr ("maroon") = Colors::maroon;
    submoduleColors.attr ("mediumaquamarine") = Colors::mediumaquamarine;
    submoduleColors.attr ("mediumblue") = Colors::mediumblue;
    submoduleColors.attr ("mediumorchid") = Colors::mediumorchid;
    submoduleColors.attr ("mediumpurple") = Colors::mediumpurple;
    submoduleColors.attr ("mediumseagreen") = Colors::mediumseagreen;
    submoduleColors.attr ("mediumslateblue") = Colors::mediumslateblue;
    submoduleColors.attr ("mediumspringgreen") = Colors::mediumspringgreen;
    submoduleColors.attr ("mediumturquoise") = Colors::mediumturquoise;
    submoduleColors.attr ("mediumvioletred") = Colors::mediumvioletred;
    submoduleColors.attr ("midnightblue") = Colors::midnightblue;
    submoduleColors.attr ("mintcream") = Colors::mintcream;
    submoduleColors.attr ("mistyrose") = Colors::mistyrose;
    submoduleColors.attr ("moccasin") = Colors::moccasin;
    submoduleColors.attr ("navajowhite") = Colors::navajowhite;
    submoduleColors.attr ("navy") = Colors::navy;
    submoduleColors.attr ("oldlace") = Colors::oldlace;
    submoduleColors.attr ("olive") = Colors::olive;
    submoduleColors.attr ("olivedrab") = Colors::olivedrab;
    submoduleColors.attr ("orange") = Colors::orange;
    submoduleColors.attr ("orangered") = Colors::orangered;
    submoduleColors.attr ("orchid") = Colors::orchid;
    submoduleColors.attr ("palegoldenrod") = Colors::palegoldenrod;
    submoduleColors.attr ("palegreen") = Colors::palegreen;
    submoduleColors.attr ("paleturquoise") = Colors::paleturquoise;
    submoduleColors.attr ("palevioletred") = Colors::palevioletred;
    submoduleColors.attr ("papayawhip") = Colors::papayawhip;
    submoduleColors.attr ("peachpuff") = Colors::peachpuff;
    submoduleColors.attr ("peru") = Colors::peru;
    submoduleColors.attr ("pink") = Colors::pink;
    submoduleColors.attr ("plum") = Colors::plum;
    submoduleColors.attr ("powderblue") = Colors::powderblue;
    submoduleColors.attr ("purple") = Colors::purple;
    submoduleColors.attr ("red") = Colors::red;
    submoduleColors.attr ("rosybrown") = Colors::rosybrown;
    submoduleColors.attr ("royalblue") = Colors::royalblue;
    submoduleColors.attr ("saddlebrown") = Colors::saddlebrown;
    submoduleColors.attr ("salmon") = Colors::salmon;
    submoduleColors.attr ("sandybrown") = Colors::sandybrown;
    submoduleColors.attr ("seagreen") = Colors::seagreen;
    submoduleColors.attr ("seashell") = Colors::seashell;
    submoduleColors.attr ("sienna") = Colors::sienna;
    submoduleColors.attr ("silver") = Colors::silver;
    submoduleColors.attr ("skyblue") = Colors::skyblue;
    submoduleColors.attr ("slateblue") = Colors::slateblue;
    submoduleColors.attr ("slategray") = Colors::slategray;
    submoduleColors.attr ("snow") = Colors::snow;
    submoduleColors.attr ("springgreen") = Colors::springgreen;
    submoduleColors.attr ("steelblue") = Colors::steelblue;
    submoduleColors.attr ("tan") = Colors::tan;
    submoduleColors.attr ("teal") = Colors::teal;
    submoduleColors.attr ("thistle") = Colors::thistle;
    submoduleColors.attr ("tomato") = Colors::tomato;
    submoduleColors.attr ("turquoise") = Colors::turquoise;
    submoduleColors.attr ("violet") = Colors::violet;
    submoduleColors.attr ("wheat") = Colors::wheat;
    submoduleColors.attr ("white") = Colors::white;
    submoduleColors.attr ("whitesmoke") = Colors::whitesmoke;
    submoduleColors.attr ("yellow") = Colors::yellow;
    submoduleColors.attr ("yellowgreen") = Colors::yellowgreen;
}

// clang-format on

} // namespace yup::Bindings
