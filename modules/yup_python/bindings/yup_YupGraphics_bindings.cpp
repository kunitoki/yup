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
        .def_static ("fromRGBA", &Color::fromRGBA)
        .def_static ("fromARGB", &Color::fromARGB)
        .def_static ("fromBGRA", &Color::fromBGRA)
        .def_static ("opaqueRandom", &Color::opaqueRandom)

        // Color data access
        .def ("getARGB", &Color::getARGB)
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

    /*
    // ============================================================================================ yup::Color

    py::class_<ColorGradient> (m, "ColorGradient")
        .def (py::init<>())
        .def (py::init<Color, float, float, Color, float, float, bool>(),
            "colour1"_a, "x1"_a, "y1"_a, "colour2"_a, "x2"_a, "y2"_a, "isRadial"_a)
        .def (py::init<Color, Point<float>, Color, Point<float>, bool>(),
            "colour1"_a, "point1"_a, "colour2"_a, "point2"_a, "isRadial"_a)
        .def (py::init<const ColorGradient&>())
        .def_static ("vertical", [](Color c1, float y1, Color c2, float y2) { return ColorGradient::vertical (c1, y1, c2, y2); },
            "colour1"_a, "y1"_a, "colour2"_a, "y2"_a)
        .def_static ("vertical", [](Color top, Color bottom, Rectangle<int> area) { return ColorGradient::vertical (top, bottom, area); },
            "colourTop"_a, "colourBottom"_a, "area"_a)
        .def_static ("vertical", [](Color top, Color bottom, Rectangle<float> area) { return ColorGradient::vertical (top, bottom, area); },
            "colourTop"_a, "colourBottom"_a, "area"_a)
        .def_static ("horizontal", [](Color c1, float x1, Color c2, float x2) { return ColorGradient::horizontal (c1, x1, c2, x2); },
            "colour1"_a, "x1"_a, "colour2"_a, "x2"_a)
        .def_static ("horizontal", [](Color left, Color right, Rectangle<int> area) { return ColorGradient::horizontal (left, right, area); },
            "colourLeft"_a, "colourRight"_a, "area"_a)
        .def_static ("horizontal", [](Color left, Color right, Rectangle<float> area) { return ColorGradient::horizontal (left, right, area); },
            "colourLeft"_a, "colourRight"_a, "area"_a)
        .def ("clearColors", &ColorGradient::clearColors)
        .def ("addColor", &ColorGradient::addColor, "proportionAlongGradient"_a, "colour"_a)
        .def ("removeColor", &ColorGradient::removeColor, "index"_a)
        .def ("multiplyOpacity", &ColorGradient::multiplyOpacity, "multiplier"_a)
        .def ("getNumColors", &ColorGradient::getNumColors)
        .def ("getColorPosition", &ColorGradient::getColorPosition, "index"_a)
        .def ("getColor", &ColorGradient::getColor, "index"_a)
        .def ("setColor", &ColorGradient::setColor, "index"_a, "colour"_a)
        .def ("getColorAtPosition", &ColorGradient::getColorAtPosition, "position"_a)
    //.def ("createLookupTable", &ColorGradient::createLookupTable)
    //.def ("createLookupTable", &ColorGradient::createLookupTable)
        .def ("isOpaque", &ColorGradient::isOpaque)
        .def ("isInvisible", &ColorGradient::isInvisible)
        .def_readwrite ("point1", &ColorGradient::point1)
        .def_readwrite ("point2", &ColorGradient::point2)
        .def_readwrite ("isRadial", &ColorGradient::isRadial)
        .def (py::self == py::self)
        .def (py::self != py::self)
    ;

    // ============================================================================================ yup::Image

    py::class_<Image> classImage (m, "Image");

    py::enum_<Image::PixelFormat> (classImage, "PixelFormat")
        .value ("UnknownFormat", Image::PixelFormat::UnknownFormat)
        .value ("RGB", Image::PixelFormat::RGB)
        .value ("ARGB", Image::PixelFormat::ARGB)
        .value ("SingleChannel", Image::PixelFormat::SingleChannel)
        .export_values();

    py::class_<Image::BitmapData> classImageBitmapData (classImage, "BitmapData", py::buffer_protocol());

    py::enum_<Image::BitmapData::ReadWriteMode> (classImageBitmapData, "ReadWriteMode")
        .value ("readOnly", Image::BitmapData::ReadWriteMode::readOnly)
        .value ("writeOnly", Image::BitmapData::ReadWriteMode::writeOnly)
        .value ("readWrite", Image::BitmapData::ReadWriteMode::readWrite)
        .export_values();

    classImageBitmapData
        .def (py::init<Image&, int, int, int, int, Image::BitmapData::ReadWriteMode>())
        .def (py::init<const Image&, int, int, int, int>())
        .def (py::init<const Image&, Image::BitmapData::ReadWriteMode>())
        .def ("getLinePointer", [](const Image::BitmapData& self, int y)
            { return py::memoryview::from_memory (self.getLinePointer(y), static_cast<Py_ssize_t> (self.size) - y * self.lineStride); })
        .def ("getPixelPointer", [](const Image::BitmapData& self, int x, int y)
            { return py::memoryview::from_memory (self.getPixelPointer(x, y), static_cast<Py_ssize_t> (self.size) - (y * self.lineStride + x * self.pixelStride)); })
        .def ("getPixelColor", &Image::BitmapData::getPixelColor)
        .def ("setPixelColor", &Image::BitmapData::setPixelColor)
        .def ("getBounds", &Image::BitmapData::getBounds)
        .def_property ("data",
            [](const Image::BitmapData& self)
                { return py::memoryview::from_memory (self.data, static_cast<Py_ssize_t> (self.size)); },
            [](Image::BitmapData& self, py::buffer data)
                { auto info = data.request(); std::memcpy (self.data, info.ptr, static_cast<size_t> (std::min (info.size, static_cast<Py_ssize_t> (self.size)))); })
        .def_readwrite ("size", &Image::BitmapData::size)
        .def_readwrite ("pixelFormat", &Image::BitmapData::pixelFormat)
        .def_readwrite ("lineStride", &Image::BitmapData::lineStride)
        .def_readwrite ("pixelStride", &Image::BitmapData::pixelStride)
        .def_readwrite ("width", &Image::BitmapData::width)
        .def_readwrite ("height", &Image::BitmapData::height)
        .def_buffer ([](Image::BitmapData& self)
        {
            return py::buffer_info
            (
                self.data,
                sizeof (unsigned char),
                py::format_descriptor<unsigned char>::format(),
                self.pixelStride,
                {
                    self.height,
                    self.width,
                    self.pixelStride
                },
                {
                    sizeof (unsigned char) * static_cast<size_t> (self.pixelStride) * static_cast<size_t> (self.width),
                    sizeof (unsigned char) * static_cast<size_t> (self.pixelStride),
                    sizeof (unsigned char)
                }
            );
        });
    ;

    classImage
        .def (py::init<>())
        .def (py::init<Image::PixelFormat, int, int, bool>())
        .def (py::init<Image::PixelFormat, int, int, bool, const ImageType&>())
        .def (py::init<const Image&>())
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def ("isValid", &Image::isValid)
        .def ("isNull", &Image::isNull)
        .def ("getWidth", &Image::getWidth)
        .def ("getHeight", &Image::getHeight)
        .def ("getBounds", &Image::getBounds)
        .def ("getFormat", &Image::getFormat)
        .def ("isARGB", &Image::isARGB)
        .def ("isRGB", &Image::isRGB)
        .def ("isSingleChannel", &Image::isSingleChannel)
        .def ("hasAlphaChannel", &Image::hasAlphaChannel)
        .def ("clear", &Image::clear)
        .def ("rescaled", &Image::rescaled)
        .def ("createCopy", &Image::createCopy)
        .def ("convertedToFormat", &Image::convertedToFormat)
        .def ("duplicateIfShared", &Image::duplicateIfShared)
        .def ("getClippedImage", &Image::getClippedImage)
        .def ("getPixelAt", &Image::getPixelAt)
        .def ("setPixelAt", &Image::setPixelAt)
        .def ("multiplyAlphaAt", &Image::multiplyAlphaAt)
        .def ("multiplyAllAlphas", &Image::multiplyAllAlphas)
        .def ("desaturate", &Image::desaturate)
        .def ("moveImageSection", &Image::moveImageSection)
        .def ("createSolidAreaMask", &Image::createSolidAreaMask)
        .def ("getProperties", &Image::getProperties, py::return_value_policy::reference_internal)
    //.def ("createLowLevelContext", &Image::createLowLevelContext)
        .def ("getReferenceCount", &Image::getReferenceCount)
        .def ("getPixelData", &Image::getPixelData, py::return_value_policy::reference_internal)
    ;

    // ============================================================================================ yup::ImagePixelData

    py::class_<ImagePixelData> classImagePixelData (m, "ImagePixelData");

    classImagePixelData
    //.def (py::init<Image::PixelFormat, int, int>())
    //.def ("createLowLevelContext", &ImagePixelData::createLowLevelContext)
    //.def ("clone", &ImagePixelData::clone)
    //.def ("createType", &ImagePixelData::createType)
    //.def ("initialiseBitmapData", &ImagePixelData::initialiseBitmapData)
        .def ("getSharedCount", &ImagePixelData::getSharedCount)
        .def_readonly ("pixelFormat", &ImagePixelData::pixelFormat)
        .def_readonly ("width", &ImagePixelData::width)
        .def_readonly ("height", &ImagePixelData::height)
        .def_readwrite ("userData", &ImagePixelData::userData)
    //.def_readwrite ("listeners", &ImagePixelData::listeners)
        .def ("sendDataChangeMessage", &ImagePixelData::sendDataChangeMessage)
    ;

    // ============================================================================================ yup::ImageType

    py::class_<ImageType, PyImageType> classImageType (m, "ImageType");

    classImageType
        .def (py::init<>())
        .def ("create", &ImageType::create)
        .def ("getTypeID", &ImageType::getTypeID)
        .def ("convert", &ImageType::convert)
    ;

    py::class_<SoftwareImageType, ImageType> classSoftwareImageType (m, "SoftwareImageType");

    classSoftwareImageType
        .def (py::init<>())
    ;

    py::class_<NativeImageType, ImageType> classNativeImageType (m, "NativeImageType");

    classNativeImageType
        .def (py::init<>())
    ;

    // ============================================================================================ yup::ImageCache

    py::class_<ImageCache, std::unique_ptr<ImageCache, py::nodelete>> classImageCache (m, "ImageCache");

    classImageCache
        .def_static ("getFromFile", &ImageCache::getFromFile)
        .def_static ("getFromMemory", [](py::buffer data)
        {
            auto info = data.request();
            return ImageCache::getFromMemory (info.ptr, static_cast<int> (info.size));
        })
        .def_static ("getFromHashCode", &ImageCache::getFromHashCode)
        .def_static ("addImageToCache", &ImageCache::addImageToCache)
        .def_static ("setCacheTimeout", &ImageCache::setCacheTimeout)
        .def_static ("releaseUnusedImages", &ImageCache::releaseUnusedImages)
    ;

    // ============================================================================================ yup::ImageCache

    py::class_<ImageFileFormat, PyImageFileFormat> classImageFileFormat (m, "ImageFileFormat");

    classImageFileFormat
        .def (py::init<>())
        .def ("getFormatName", &ImageFileFormat::getFormatName)
        .def ("canUnderstand", &ImageFileFormat::canUnderstand)
        .def ("usesFileExtension", &ImageFileFormat::usesFileExtension)
        .def ("decodeImage", &ImageFileFormat::decodeImage)
        .def ("writeImageToStream", &ImageFileFormat::writeImageToStream)
        .def_static ("findImageFormatForStream", &ImageFileFormat::findImageFormatForStream, py::return_value_policy::reference_internal)
        .def_static ("findImageFormatForFileExtension", &ImageFileFormat::findImageFormatForFileExtension, py::return_value_policy::reference_internal)
        .def_static ("loadFrom", static_cast<Image (*)(InputStream&)> (&ImageFileFormat::loadFrom))
        .def_static ("loadFrom", static_cast<Image (*)(const File&)> (&ImageFileFormat::loadFrom))
        .def_static ("loadFrom", [](py::buffer data)
        {
            auto info = data.request();
            return ImageFileFormat::loadFrom (info.ptr, static_cast<size_t> (info.size));
        })
    ;

    py::class_<PNGImageFormat, ImageFileFormat> classPNGImageFormat (m, "PNGImageFormat");
    classPNGImageFormat
        .def (py::init<>());

    py::class_<JPEGImageFormat, ImageFileFormat> classJPEGImageFormat (m, "JPEGImageFormat");
    classJPEGImageFormat
        .def (py::init<>());

    py::class_<GIFImageFormat, ImageFileFormat> classGIFImageFormat (m, "GIFImageFormat");
    classGIFImageFormat
        .def (py::init<>());

    // ============================================================================================ yup::ScaledImage

    py::class_<ScaledImage> classScaledImage (m, "ScaledImage");

    classScaledImage
        .def (py::init<>())
        .def (py::init<const Image&>())
        .def (py::init<const Image&, double>())
        .def (py::init<const ScaledImage&>())
        .def ("getImage", &ScaledImage::getImage)
        .def ("getScale", &ScaledImage::getScale)
        .def ("getScaledBounds", &ScaledImage::getScaledBounds)
    ;

    // ============================================================================================ yup::ScaledImage

    py::class_<ImageConvolutionKernel> classImageConvolutionKernel (m, "ImageConvolutionKernel");

    classImageConvolutionKernel
        .def (py::init<int>(), "size"_a)
        .def ("clear", &ImageConvolutionKernel::clear)
        .def ("getKernelValue", &ImageConvolutionKernel::getKernelValue)
        .def ("setKernelValue", &ImageConvolutionKernel::setKernelValue)
        .def ("setOverallSum", &ImageConvolutionKernel::setOverallSum)
        .def ("rescaleAllValues", &ImageConvolutionKernel::rescaleAllValues)
        .def ("createGaussianBlur", &ImageConvolutionKernel::createGaussianBlur)
        .def ("getKernelSize", &ImageConvolutionKernel::getKernelSize)
        .def ("applyToImage", &ImageConvolutionKernel::applyToImage)
    ;

    // ============================================================================================ yup::Font

    py::class_<Font> classFont (m, "Font");
    py::class_<FontOptions> classFontOptions (m, "FontOptions");

    Helpers::makeArithmeticEnum<Font::FontStyleFlags> (classFont, "FontStyleFlags")
        .value ("plain", Font::FontStyleFlags::plain)
        .value ("bold", Font::FontStyleFlags::bold)
        .value ("italic", Font::FontStyleFlags::italic)
        .value ("underlined", Font::FontStyleFlags::underlined)
        .export_values();

    classFontOptions
        .def (py::init<>())
        .def (py::init<float>(), "fontHeight"_a)
        .def (py::init<float, int>(), "fontHeight"_a, "styleFlags"_a = Font::plain)
        .def (py::init<float, Font::FontStyleFlags>(), "fontHeight"_a, "styleFlags"_a)
        .def (py::init<const String&, float, Font::FontStyleFlags>(), "typefaceName"_a, "fontHeight"_a, "styleFlags"_a)
        .def (py::init<const String&, const String&, float>(), "typefaceName"_a, "typefaceStyle"_a, "styleFlags"_a)
    //.def (py::init<const Typeface::Ptr&>())
        .def (py::init<const FontOptions&>())
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def (py::self < py::self)
        .def (py::self <= py::self)
        .def (py::self > py::self)
        .def (py::self >= py::self)
        .def ("withName", &FontOptions::withName)
        .def ("withStyle", &FontOptions::withStyle)
        .def ("withTypeface", &FontOptions::withTypeface)
        .def ("withFallbacks", &FontOptions::withFallbacks)
        .def ("withFallbackEnabled", &FontOptions::withFallbackEnabled, "x"_a = true)
        .def ("withHeight", &FontOptions::withHeight, "x"_a)
        .def ("withPointHeight", &FontOptions::withPointHeight, "x"_a)
        .def ("withKerningFactor", &FontOptions::withKerningFactor, "x"_a)
        .def ("withHorizontalScale", &FontOptions::withHorizontalScale, "x"_a)
        .def ("withUnderline", &FontOptions::withUnderline, "x"_a = true)
    //.def ("withMetricsKind", &FontOptions::withMetricsKind)
        .def ("getName", &FontOptions::getName)
        .def ("getStyle", &FontOptions::getStyle)
        .def ("getTypeface", &FontOptions::getTypeface)
        .def ("getFallbacks", &FontOptions::getFallbacks)
        .def ("getHeight", &FontOptions::getHeight)
        .def ("getPointHeight", &FontOptions::getPointHeight)
        .def ("getKerningFactor", &FontOptions::getKerningFactor)
        .def ("getHorizontalScale", &FontOptions::getHorizontalScale)
        .def ("getFallbackEnabled", &FontOptions::getFallbackEnabled)
        .def ("getUnderline", &FontOptions::getUnderline)
    //.def ("getMetricsKind", &FontOptions::getMetricsKind)
    ;

    classFont
        .def (py::init<FontOptions>(), "fontOptions"_a)
        .def (py::init<const Font&>())
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def ("setTypefaceName", &Font::setTypefaceName)
        .def ("getTypefaceName", &Font::getTypefaceName)
        .def ("getTypefaceStyle", &Font::getTypefaceStyle)
        .def ("setTypefaceStyle", &Font::setTypefaceStyle)
        .def ("withTypefaceStyle", &Font::withTypefaceStyle)
        .def_static ("getDefaultSansSerifFontName", &Font::getDefaultSansSerifFontName)
        .def_static ("getDefaultSerifFontName", &Font::getDefaultSerifFontName)
        .def_static ("getDefaultMonospacedFontName", &Font::getDefaultMonospacedFontName)
        .def_static ("getDefaultStyle", &Font::getDefaultStyle)
        .def ("withHeight", &Font::withHeight)
        .def ("withPointHeight", &Font::withPointHeight)
        .def ("setHeight", &Font::setHeight)
        .def ("setHeightWithoutChangingWidth", &Font::setHeightWithoutChangingWidth)
        .def ("getHeight", &Font::getHeight)
        .def ("getHeightInPoints", &Font::getHeightInPoints)
        .def ("getAscent", &Font::getAscent)
        .def ("getAscentInPoints", &Font::getAscentInPoints)
        .def ("getDescent", &Font::getDescent)
        .def ("getDescentInPoints", &Font::getDescentInPoints)
        .def ("getStyleFlags", &Font::getStyleFlags)
        .def ("withStyle", &Font::withStyle)
        .def ("withStyle", [](const Font& self, Font::FontStyleFlags flags) { return self.withStyle (static_cast<int> (flags)); })
        .def ("setStyleFlags", &Font::setStyleFlags)
        .def ("setStyleFlags", [](Font& self, Font::FontStyleFlags flags) { self.setStyleFlags (static_cast<int> (flags)); })
        .def ("setBold", &Font::setBold)
        .def ("boldened", &Font::boldened)
        .def ("isBold", &Font::isBold)
        .def ("setItalic", &Font::setItalic)
        .def ("italicised", &Font::italicised)
        .def ("isItalic", &Font::isItalic)
        .def ("setUnderline", &Font::setUnderline)
        .def ("isUnderlined", &Font::isUnderlined)
        .def ("getHorizontalScale", &Font::getHorizontalScale)
        .def ("withHorizontalScale", &Font::withHorizontalScale)
        .def ("setHorizontalScale", &Font::setHorizontalScale)
        .def_static ("getDefaultMinimumHorizontalScaleFactor", &Font::getDefaultMinimumHorizontalScaleFactor)
        .def_static ("setDefaultMinimumHorizontalScaleFactor", &Font::setDefaultMinimumHorizontalScaleFactor)
        .def ("getExtraKerningFactor", &Font::getExtraKerningFactor)
        .def ("withExtraKerningFactor", &Font::withExtraKerningFactor)
        .def ("setExtraKerningFactor", &Font::setExtraKerningFactor)
        .def ("setSizeAndStyle", py::overload_cast<float, int, float, float> (&Font::setSizeAndStyle))
        .def ("setSizeAndStyle", [](Font& self, float newHeight, Font::FontStyleFlags newStyleFlags, float newHorizontalScale, float newKerningAmount)
        {
            self.setSizeAndStyle (newHeight, static_cast<int> (newStyleFlags), newHorizontalScale, newKerningAmount);
        })
        .def ("setSizeAndStyle", py::overload_cast<float, const String&, float, float> (&Font::setSizeAndStyle))
        .def ("getStringWidth", &Font::getStringWidth)
        .def ("getStringWidthFloat", &Font::getStringWidthFloat)
    //.def ("getGlyphPositions", &Font::getGlyphPositions)
    //.def ("getTypefacePtr", &Font::getTypefacePtr)
    //.def_static ("findFonts", &Font::findFonts)
        .def_static ("findAllTypefaceNames", &Font::findAllTypefaceNames)
        .def_static ("findAllTypefaceStyles", &Font::findAllTypefaceStyles)
    //.def_static ("getFallbackFontName", &Font::getFallbackFontName)
    //.def_static ("setFallbackFontName", &Font::setFallbackFontName)
    //.def_static ("getFallbackFontStyle", &Font::getFallbackFontStyle)
    //.def_static ("setFallbackFontStyle", &Font::setFallbackFontStyle)
        .def ("toString", &Font::toString)
        .def_static ("fromString", &Font::fromString)
        .def ("__repr__", [](const Font& self)
        {
            String repr;
            repr << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name()) << "('" << self.toString() << ")";
            return repr;
        })
        .def ("__str__", &Font::toString)
    ;

    // ============================================================================================ yup::AttributedString

    py::class_<AttributedString> classAttributedString (m, "AttributedString");

    py::enum_<AttributedString::WordWrap> (classAttributedString, "WordWrap")
        .value("none", AttributedString::WordWrap::none)
        .value("byWord", AttributedString::WordWrap::byWord)
        .value("byChar", AttributedString::WordWrap::byChar)
        .export_values();

    py::enum_<AttributedString::ReadingDirection> (classAttributedString, "ReadingDirection")
        .value("natural", AttributedString::ReadingDirection::natural)
        .value("leftToRight", AttributedString::ReadingDirection::leftToRight)
        .value("rightToLeft", AttributedString::ReadingDirection::rightToLeft)
        .export_values();

    py::class_<AttributedString::Attribute> (classAttributedString, "Attribute")
        .def (py::init<>())
        .def (py::init<Range<int>, const Font&, Color>())
        .def (py::init<const AttributedString::Attribute&>())
        .def_readwrite ("range", &AttributedString::Attribute::range)
        .def_readwrite ("font", &AttributedString::Attribute::font)
        .def_readwrite ("colour", &AttributedString::Attribute::colour)
    ;

    classAttributedString
        .def (py::init<>())
        .def (py::init<const String&>())
        .def (py::init<const AttributedString&>())
        .def ("getText", &AttributedString::getText)
        .def ("setText", &AttributedString::setText)
        .def ("append", py::overload_cast<const String&> (&AttributedString::append))
        .def ("append", py::overload_cast<const String&, const Font&> (&AttributedString::append))
        .def ("append", py::overload_cast<const String&, Color> (&AttributedString::append))
        .def ("append", py::overload_cast<const String&, const Font&, Color> (&AttributedString::append))
        .def ("append", py::overload_cast<const AttributedString&> (&AttributedString::append))
        .def ("clear", &AttributedString::clear)
        .def ("draw", &AttributedString::draw)
        .def ("getJustification", &AttributedString::getJustification)
        .def ("getWordWrap", &AttributedString::getWordWrap)
        .def ("setWordWrap", &AttributedString::setWordWrap)
        .def ("getReadingDirection", &AttributedString::getReadingDirection)
        .def ("setReadingDirection", &AttributedString::setReadingDirection)
        .def ("getLineSpacing", &AttributedString::getLineSpacing)
        .def ("setLineSpacing", &AttributedString::setLineSpacing)
        .def ("getNumAttributes", &AttributedString::getNumAttributes)
        .def ("getAttribute", &AttributedString::getAttribute, py::return_value_policy::reference)
        .def ("setColor", py::overload_cast<Range<int>, Color> (&AttributedString::setColor))
        .def ("setColor", py::overload_cast<Color> (&AttributedString::setColor))
        .def ("setFont", py::overload_cast<Range<int>, const Font&> (&AttributedString::setFont))
        .def ("setFont", py::overload_cast<const Font&> (&AttributedString::setFont))
    ;

    // ============================================================================================ yup::FillType

    py::class_<FillType> classFillType (m, "FillType");

    classFillType
        .def (py::init<>())
        .def (py::init<Color>())
        .def (py::init<const ColorGradient&>())
        .def (py::init<const Image&, const AffineTransform&>())
        .def (py::init<const FillType&>())
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def ("isColor", &FillType::isColor)
        .def ("isGradient", &FillType::isGradient)
        .def ("isTiledImage", &FillType::isTiledImage)
        .def ("setColor", &FillType::setColor)
        .def ("setGradient", &FillType::setGradient)
        .def ("setTiledImage", &FillType::setTiledImage)
        .def ("setOpacity", &FillType::setOpacity)
        .def ("getOpacity", &FillType::getOpacity)
        .def ("isInvisible", &FillType::isInvisible)
        .def ("transformed", &FillType::transformed)
        .def_readwrite("colour", &FillType::colour)
        .def_property("gradient",
                      [](const FillType& self) { return self.gradient.get(); },
                      [](FillType& self, ColorGradient* v) { self.gradient.reset(); if (v != nullptr) self.gradient = std::make_unique<ColorGradient>(*v); },
                      py::return_value_policy::reference_internal)
        .def_readwrite("image", &FillType::image)
        .def_readwrite("transform", &FillType::transform)
    ;

    // ============================================================================================ yup::RectanglePlacement

    py::class_<RectanglePlacement> classRectanglePlacement (m, "RectanglePlacement");

    Helpers::makeArithmeticEnum<RectanglePlacement::Flags> (classRectanglePlacement, "Flags")
        .value ("xLeft", RectanglePlacement::Flags::xLeft)
        .value ("xRight", RectanglePlacement::Flags::xRight)
        .value ("xMid", RectanglePlacement::Flags::xMid)
        .value ("yTop", RectanglePlacement::Flags::yTop)
        .value ("yBottom", RectanglePlacement::Flags::yBottom)
        .value ("yMid", RectanglePlacement::Flags::yMid)
        .value ("stretchToFit", RectanglePlacement::Flags::stretchToFit)
        .value ("fillDestination", RectanglePlacement::Flags::fillDestination)
        .value ("onlyReduceInSize", RectanglePlacement::Flags::onlyReduceInSize)
        .value ("onlyIncreaseInSize", RectanglePlacement::Flags::onlyIncreaseInSize)
        .value ("doNotResize", RectanglePlacement::Flags::doNotResize)
        .value ("centred", RectanglePlacement::Flags::centred)
        .export_values();

    classRectanglePlacement
        .def (py::init<>())
        .def (py::init<int>())
        .def (py::init<RectanglePlacement::Flags>())
        .def (py::init<const RectanglePlacement&>())
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def ("getFlags", &RectanglePlacement::getFlags)
        .def ("testFlags", &RectanglePlacement::testFlags)
        .def ("testFlags", [](const RectanglePlacement& self, RectanglePlacement::Flags flags) { return self.testFlags (static_cast<int> (flags)); })
        .def ("applyTo", [](const RectanglePlacement& self, double& sourceX, double& sourceY, double& sourceW, double& sourceH, double destinationX, double destinationY, double destinationW, double destinationH)
        {
            self.applyTo (sourceX, sourceY, sourceW, sourceH, destinationX, destinationY, destinationW, destinationH);
            return py::make_tuple (sourceX, sourceY, sourceW, sourceH);
        })
        .def ("appliedTo", &RectanglePlacement::template appliedTo<int>)
        .def ("appliedTo", &RectanglePlacement::template appliedTo<float>)
        .def ("getTransformToFit", &RectanglePlacement::getTransformToFit)
    ;

    py::implicitly_convertible<RectanglePlacement::Flags, RectanglePlacement>();

    // ============================================================================================ yup::LowLevelGraphicsContext

    py::class_<LowLevelGraphicsContext, PyLowLevelGraphicsContext<>> classLowLevelGraphicsContext (m, "LowLevelGraphicsContext");

    classLowLevelGraphicsContext
        .def (py::init<>())
        // TODO
    ;

    // ============================================================================================ yup::LowLevelGraphicsSoftwareRenderer

    py::class_<LowLevelGraphicsSoftwareRenderer, LowLevelGraphicsContext, PyLowLevelGraphicsContext<LowLevelGraphicsSoftwareRenderer>>
        classLowLevelGraphicsSoftwareRenderer (m, "LowLevelGraphicsSoftwareRenderer");

    classLowLevelGraphicsSoftwareRenderer
        .def (py::init<const Image&>(), "imageToRenderOnto"_a)
        .def (py::init<const Image&, Point<int>, const RectangleList<int>&>(), "imageToRenderOnto"_a, "origin"_a, "initialClip"_a)
    ;

    */

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
        }).def ("__exit__", [] (PyGraphicsSaveState& self, const std::optional<py::type>&, const std::optional<py::object>&, const std::optional<py::object>&)
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
        .def ("fillEllipse", &Graphics::fillEllipse)

        // Path operations
        .def ("strokePath", &Graphics::strokePath)
        .def ("fillPath", &Graphics::fillPath)

        // Image operations
        .def ("drawImageAt", &Graphics::drawImageAt)

        // Text operations
        .def ("fillFittedText", &Graphics::fillFittedText)
        .def ("strokeFittedText", &Graphics::strokeFittedText)

        // State management
        .def ("saveState", [](Graphics& self) { return PyGraphicsSaveState{self}; })

        // Utility methods
        .def ("getContextScale", &Graphics::getContextScale)
        .def ("getFactory", &Graphics::getFactory, py::return_value_policy::reference_internal)
        .def ("getRenderer", &Graphics::getRenderer, py::return_value_policy::reference_internal)
    ;

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
