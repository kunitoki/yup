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

namespace yup
{

//==============================================================================

namespace
{

// How long a step button is held before it steps again, then between its steps
constexpr int stepRepeatDelayMs = 400;
constexpr int stepRepeatIntervalMs = 80;

struct SliderIds
{
    static inline const Identifier sliderType { "sliderType" };
    static inline const Identifier minimum { "minimum" };
    static inline const Identifier maximum { "maximum" };
    static inline const Identifier interval { "interval" };
    static inline const Identifier skew { "skew" };
    static inline const Identifier defaultValue { "defaultValue" };
    static inline const Identifier value { "value" };
    static inline const Identifier normalized { "normalized" };
    static inline const Identifier minValue { "minValue" };
    static inline const Identifier maxValue { "maxValue" };
    static inline const Identifier minNormalized { "minNormalized" };
    static inline const Identifier maxNormalized { "maxNormalized" };
    static inline const Identifier dragging { "dragging" };
    static inline const Identifier thumbSize { "thumbSize" };
    static inline const Identifier trackBounds { "trackBounds" };
    static inline const Identifier thumbBounds { "thumbBounds" };
    static inline const Identifier decrementBounds { "decrementBounds" };
    static inline const Identifier incrementBounds { "incrementBounds" };
    static inline const Identifier pressedStep { "pressedStep" };
    static inline const Identifier decimalPlaces { "decimalPlaces" };
    static inline const Identifier dragSensitivity { "dragSensitivity" };
    static inline const Identifier fineDragScale { "fineDragScale" };
    static inline const Identifier fineDragModifier { "fineDragModifier" };
    static inline const Identifier resetModifier { "resetModifier" };
};

// True while all the keys asked for are held, and never when none is asked for
bool areModifiersDown (KeyModifiers wanted, const KeyModifiers& held)
{
    return wanted.getFlags() != 0 && (held.getFlags() & wanted.getFlags()) == wanted.getFlags();
}

// A property naming one modifier key, whose default is not the first of the names as the one of an enumeration is
template <class Getter, class Setter>
ComponentProperty makeModifierProperty (const Identifier& name, Getter getter, Setter setter, const String& defaultName, String description)
{
    const StringArray names { "none", "shift", "ctrl", "alt", "command" };
    const std::vector<int> masks { 0, KeyModifiers::shiftMask, KeyModifiers::controlMask, KeyModifiers::altMask, KeyModifiers::commandMask };

    return {
        .name = name,
        .type = ComponentProperty::Type::enumeration,
        .defaultValue = defaultName,
        .getter = [names, masks, getter] (const Component& component)
        {
            const auto flags = std::invoke (getter, static_cast<const Slider&> (component)).getFlags();
            const auto found = std::find (masks.begin(), masks.end(), flags);

            // Keys given together in code have no name
            return var (found != masks.end() ? names[static_cast<int> (found - masks.begin())] : String());
        },
        .setter = [names, masks, setter] (Component& component, const var& value)
        {
            if (const auto index = names.indexOf (value.toString()); index >= 0)
                std::invoke (setter, static_cast<Slider&> (component), KeyModifiers (masks[static_cast<size_t> (index)]));
        },
        .isStyleable = false,
        .enumValues = names,
        .description = std::move (description)
    };
}

void setSliderRangeBound (Slider& slider, double bound, bool isMinimum)
{
    if (! std::isfinite (bound))
        return;

    auto range = slider.getRange();
    const auto length = range.end - range.start;

    if (isMinimum)
    {
        range.start = bound;
        range.end = range.end > bound ? range.end : bound + length;
    }
    else
    {
        range.end = bound;
        range.start = range.start < bound ? range.start : bound - length;
    }

    slider.setRange (range);
}

bool isLinearSlider (Slider::SliderType type)
{
    return type == Slider::LinearHorizontal || type == Slider::LinearVertical || type == Slider::LinearBarHorizontal || type == Slider::LinearBarVertical;
}

bool isTwoValueSlider (Slider::SliderType type)
{
    return type == Slider::TwoValueHorizontal || type == Slider::TwoValueVertical;
}

bool isVerticalSlider (Slider::SliderType type)
{
    return type == Slider::LinearVertical || type == Slider::LinearBarVertical || type == Slider::TwoValueVertical;
}

} // namespace

//==============================================================================

const Identifier Slider::Style::backgroundColorId { "sliderBackgroundColorId" };
const Identifier Slider::Style::trackColorId { "sliderTrackColorId" };
const Identifier Slider::Style::thumbColorId { "sliderThumbColorId" };
const Identifier Slider::Style::thumbOverColorId { "sliderThumbOverColorId" };
const Identifier Slider::Style::thumbDownColorId { "sliderThumbDownColorId" };
const Identifier Slider::Style::textColorId { "sliderTextColorId" };
const Identifier Slider::Style::trackWidthId { "sliderTrackWidthId" };
const Identifier Slider::Style::thumbSizeId { "sliderThumbSizeId" };
const Identifier Slider::Style::rotaryInsetId { "sliderRotaryInsetId" };
const Identifier Slider::Style::rotaryDialInsetId { "sliderRotaryDialInsetId" };
const Identifier Slider::Style::rotaryDialOutlineWidthId { "sliderRotaryDialOutlineWidthId" };
const Identifier Slider::Style::rotaryPointerInsetId { "sliderRotaryPointerInsetId" };
const Identifier Slider::Style::rotaryPointerWidthId { "sliderRotaryPointerWidthId" };
const Identifier Slider::Style::rotaryArcWidthId { "sliderRotaryArcWidthId" };

//==============================================================================

const ComponentTypeInfo& Slider::getClassTypeInfo()
{
    // The range comes before the values it constrains, the builder applies values in declaration order
    static const auto info = ComponentTypeBuilder<Slider> (Identifier ("Slider"))
                                 .factory ([] { return std::make_unique<Slider> (LinearHorizontal); })
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("trackColor", Style::trackColorId)
                                 .color ("thumbColor", Style::thumbColorId)
                                 .color ("thumbOverColor", Style::thumbOverColorId)
                                 .color ("thumbDownColor", Style::thumbDownColorId)
                                 .color ("textColor", Style::textColorId)
                                 .number ("trackWidth", Style::trackWidthId, 4.0, "The thickness of the linear track.")
                                 .number ("thumbSize", Style::thumbSizeId, 16.0, "The diameter of the linear thumbs.")
                                 .number ("rotaryInset", Style::rotaryInsetId, 0.1, "The inset of the rotary dial, as a proportion of the smaller side.")
                                 .number ("rotaryDialInset", Style::rotaryDialInsetId, 0.105, "The inset of the rotary dial body inside the arc bounds, as a proportion of the smaller side.")
                                 .number ("rotaryDialOutlineWidth", Style::rotaryDialOutlineWidthId, 0.0175, "The stroke width of the rotary dial outline, as a proportion of the smaller side.")
                                 .number ("rotaryPointerInset", Style::rotaryPointerInsetId, 0.175, "The inset of the rotary pointer tip inside the arc bounds, as a proportion of the smaller side.")
                                 .number ("rotaryPointerWidth", Style::rotaryPointerWidthId, 0.03, "The stroke width of the rotary pointer, as a proportion of the smaller side.")
                                 .number ("rotaryArcWidth", Style::rotaryArcWidthId, 0.075, "The stroke width of the rotary track and value arcs, as a proportion of the smaller side.")
                                 .enumeration (
                                     SliderIds::sliderType,
                                     { "linearHorizontal", "linearVertical", "linearBarHorizontal", "linearBarVertical", "rotary", "rotaryHorizontalDrag",
                                       "rotaryVerticalDrag", "incDecButtons", "twoValueHorizontal", "twoValueVertical" },
                                     &Slider::getSliderType,
                                     &Slider::setSliderType)
                                 .property (SliderIds::minimum, [] (const Slider& s) { return s.getRange().start; }, [] (Slider& s, double v) { setSliderRangeBound (s, v, true); }, 0.0, "The lowest value of the range.")
                                 .property (SliderIds::maximum, [] (const Slider& s) { return s.getRange().end; }, [] (Slider& s, double v) { setSliderRangeBound (s, v, false); }, 1.0, "The highest value of the range.")
                                 .property (SliderIds::interval, &Slider::getInterval, [] (Slider& s, double v)
                                 {
                                     if (v >= 0.0)
                                     {
                                         auto range = s.getRange();
                                         range.interval = v;
                                         s.setRange (range);
                                     }
                                 }, 0.0, "The step between values, 0 for continuous.")
                                 .property (SliderIds::skew, &Slider::getSkewFactor, [] (Slider& s, double v)
                                 {
                                     if (v > 0.0)
                                         s.setSkewFactor (v);
                                 }, 1.0, "The skew factor of the range, 1 for linear, below 1 to give the lower values more space.")
                                 .property (SliderIds::defaultValue, &Slider::getDefaultValue, &Slider::setDefaultValue, 0.0, "The value a double-click, or a click with the reset modifier, resets to.")
                                 .property (SliderIds::value, &Slider::getValue, [] (Slider& s, double v) { s.setValue (v); })
                                 .readOnly (SliderIds::normalized, &Slider::getValueNormalised)
                                 .property (SliderIds::minValue, &Slider::getMinValue, [] (Slider& s, double v) { s.setMinValue (v); })
                                 .property (SliderIds::maxValue, &Slider::getMaxValue, [] (Slider& s, double v) { s.setMaxValue (v); })
                                 .readOnly (SliderIds::minNormalized, &Slider::getMinValueNormalised, 0.0, "Where the lower value of a two-value slider is along the range, from 0 to 1.")
                                 .readOnly (SliderIds::maxNormalized, &Slider::getMaxValueNormalised, 1.0, "Where the upper value of a two-value slider is along the range, from 0 to 1.")
                                 .state (SliderIds::dragging, &Slider::isCurrentlyBeingDragged, false)
                                 .readOnly (SliderIds::trackBounds, &Slider::getTrackBounds, {}, "The area the value moves in: the track of a linear slider, the square of a rotary one.")
                                 .readOnly (SliderIds::thumbBounds, &Slider::getThumbBounds, {}, "The area of the thumb of a linear slider, centered where the value is along the track and of the size of thumbSize: what a press grabs. Empty for the others.")
                                 .readOnly (SliderIds::decrementBounds, &Slider::getDecrementBounds, {}, "The area of the button lowering the value of an incDecButtons slider, empty for the others.")
                                 .readOnly (SliderIds::incrementBounds, &Slider::getIncrementBounds, {}, "The area of the button raising the value of an incDecButtons slider, empty for the others.")
                                 .readOnly (SliderIds::pressedStep, &Slider::getPressedStep, 0, "The button of an incDecButtons slider that is pressed: -1 for the one lowering the value, 1 for the one raising it, 0 for none.")
                                 .property (SliderIds::decimalPlaces, &Slider::getNumDecimalPlacesToDisplay, &Slider::setNumDecimalPlacesToDisplay, 7, "The decimal places the value is shown with, 0 or less for as many as it needs. The default theme shows 3 at most.")
                                 .property (SliderIds::dragSensitivity, &Slider::getMouseDragSensitivity, &Slider::setMouseDragSensitivity, 1.0, "How far the value moves for a drag: 1 crosses the range in the length of the slider. A new rotary slider has 0.3.")
                                 .property (SliderIds::fineDragScale, &Slider::getFineDragScale, &Slider::setFineDragScale, 0.1, "What the sensitivity of a drag and of the wheel is multiplied by while the fine drag modifier is held.")
                                 .addProperty (makeModifierProperty (SliderIds::fineDragModifier, &Slider::getFineDragModifiers, &Slider::setFineDragModifiers, "shift", "The key that makes a drag and the wheel fine while it is held."))
                                 .addProperty (makeModifierProperty (SliderIds::resetModifier, &Slider::getResetModifiers, &Slider::setResetModifiers, "alt", "The key a click is held with to bring the default value back."))
                                 .signal ("onValueChanged", &Slider::onValueChanged, "Emitted with the new value when the value changes", { { Identifier ("value"), ComponentProperty::Type::number } })
                                 .build();

    return info;
}

//==============================================================================

Slider::Slider (SliderType sliderType, StringRef componentID)
    : Component (componentID)
    , sliderType (sliderType)
    , currentValue (defaultValue)
    , stepRepeater (bindFront (&Slider::repeatStep, this))
{
    setMouseCursor (MouseCursor::Hand);
    setWantsKeyboardFocus (true);
    setOpaque (false);

    if (sliderType == Rotary || sliderType == RotaryHorizontalDrag || sliderType == RotaryVerticalDrag)
        setMouseDragSensitivity (0.3f);

    setValue (defaultValue, dontSendNotification);
}

Slider::Slider (SliderType sliderType)
    : Slider (sliderType, {})
{
}

//==============================================================================

void Slider::setValue (double newValue, NotificationType notification)
{
    newValue = constrainValue (newValue);

    if (! approximatelyEqual (currentValue, newValue))
    {
        currentValue = newValue;

        const BailOutChecker bailOutChecker (this);

        notifyPropertyChanged (SliderIds::value);
        if (bailOutChecker.shouldBailOut())
            return;

        notifyPropertyChanged (SliderIds::normalized);
        if (bailOutChecker.shouldBailOut())
            return;

        sendValueChanged (notification);

        if (! bailOutChecker.shouldBailOut())
            repaint();
    }
}

double Slider::getValue() const
{
    return currentValue;
}

void Slider::setValueNormalised (double newValue, NotificationType notification)
{
    setValue (range.convertFrom0to1 (std::clamp (newValue, 0.0, 1.0)), notification);
}

double Slider::getValueNormalised() const
{
    return range.convertTo0to1 (currentValue);
}

void Slider::valueChanged() {}

//==============================================================================

void Slider::setMinValue (double newMinValue, NotificationType notification, bool allowNudgingOfOtherValues)
{
    newMinValue = constrainValue (newMinValue);

    if (allowNudgingOfOtherValues)
    {
        if (newMinValue > maxValue)
            setMaxValue (newMinValue, notification, false);

        if (newMinValue > currentValue)
            setValue (newMinValue, notification);
    }

    if (! approximatelyEqual (minValue, newMinValue))
    {
        minValue = newMinValue;

        const BailOutChecker bailOutChecker (this);

        notifyPropertyChanged (SliderIds::minValue);
        if (bailOutChecker.shouldBailOut())
            return;

        notifyPropertyChanged (SliderIds::minNormalized);
        if (bailOutChecker.shouldBailOut())
            return;

        sendMinValueChanged (notification);

        if (! bailOutChecker.shouldBailOut())
            repaint();
    }
}

double Slider::getMinValue() const
{
    return minValue;
}

double Slider::getMinValueNormalised() const
{
    return range.convertTo0to1 (minValue);
}

void Slider::setMaxValue (double newMaxValue, NotificationType notification, bool allowNudgingOfOtherValues)
{
    newMaxValue = constrainValue (newMaxValue);

    if (allowNudgingOfOtherValues)
    {
        if (newMaxValue < minValue)
            setMinValue (newMaxValue, notification, false);

        if (newMaxValue < currentValue)
            setValue (newMaxValue, notification);
    }

    if (! approximatelyEqual (maxValue, newMaxValue))
    {
        maxValue = newMaxValue;

        const BailOutChecker bailOutChecker (this);

        notifyPropertyChanged (SliderIds::maxValue);
        if (bailOutChecker.shouldBailOut())
            return;

        notifyPropertyChanged (SliderIds::maxNormalized);
        if (bailOutChecker.shouldBailOut())
            return;

        sendMaxValueChanged (notification);

        if (! bailOutChecker.shouldBailOut())
            repaint();
    }
}

double Slider::getMaxValue() const
{
    return maxValue;
}

double Slider::getMaxValueNormalised() const
{
    return range.convertTo0to1 (maxValue);
}

void Slider::minValueChanged() {}

void Slider::maxValueChanged() {}

//==============================================================================

void Slider::setDefaultValue (double newDefaultValue)
{
    newDefaultValue = constrainValue (newDefaultValue);

    if (! approximatelyEqual (defaultValue, newDefaultValue))
    {
        defaultValue = newDefaultValue;
        notifyPropertyChanged (SliderIds::defaultValue);
    }
}

double Slider::getDefaultValue() const
{
    return defaultValue;
}

//==============================================================================

void Slider::setRange (const NormalisableRange<double>& newRange)
{
    if (! (newRange.end > newRange.start))
    {
        jassertfalse; // An empty or reversed range has no position to give the values
        return;
    }

    const auto previousRange = range;
    const auto previousValue = currentValue;

    range = newRange;
    setDefaultValue (constrainValue (defaultValue));
    setValue (constrainValue (currentValue), dontSendNotification);
    setMinValue (constrainValue (minValue), dontSendNotification);
    setMaxValue (constrainValue (maxValue), dontSendNotification);

    const BailOutChecker bailOutChecker (this);

    sendRangeChanged (previousRange);

    if (! bailOutChecker.shouldBailOut()
        && approximatelyEqual (previousValue, currentValue)
        && ! approximatelyEqual (previousRange.convertTo0to1 (currentValue), range.convertTo0to1 (currentValue)))
    {
        notifyPropertyChanged (SliderIds::normalized);
    }
}

void Slider::setRange (double minValue, double maxValue, double stepSize)
{
    setRange (NormalisableRange<double> (minValue, maxValue, stepSize));
}

NormalisableRange<double> Slider::getRange() const
{
    return range;
}

double Slider::getInterval() const
{
    return range.interval;
}

//==============================================================================

void Slider::setSkewFactor (double skewFactor)
{
    if (skewFactor <= 0.0)
    {
        jassertfalse; // Skew factor must be positive
        return;
    }

    if (! approximatelyEqual (range.skew, skewFactor))
    {
        auto updatedRange = range;
        updatedRange.skew = skewFactor;
        setRange (updatedRange);
        repaint();
    }
}

void Slider::setSkewFactorFromMidpoint (double midpointValue)
{
    midpointValue = jlimit (range.getRange().getStart(), range.getRange().getEnd(), midpointValue);

    auto updatedRange = range;
    updatedRange.setSkewForCentre (midpointValue);
    setRange (updatedRange);
    repaint();
}

double Slider::getSkewFactor() const
{
    return range.skew;
}

//==============================================================================

void Slider::setNumDecimalPlacesToDisplay (int decimalPlaces)
{
    if (numDecimalPlaces == decimalPlaces)
        return;

    numDecimalPlaces = decimalPlaces;
    repaint();
    notifyPropertyChanged (SliderIds::decimalPlaces);
}

int Slider::getNumDecimalPlacesToDisplay() const
{
    return numDecimalPlaces;
}

//==============================================================================

void Slider::setSliderType (SliderType newType)
{
    if (sliderType != newType)
    {
        const BailOutChecker bailOutChecker (this);

        // What a press began on the slider as it was has nothing to go on with
        cancelInteraction();

        if (bailOutChecker.shouldBailOut())
            return;

        sliderType = newType;
        notifyPropertyChanged (SliderIds::sliderType);
        repaint();
        resized();
    }
}

Slider::SliderType Slider::getSliderType() const
{
    return sliderType;
}

void Slider::setTextBoxStyle (TextEntryBoxPosition position, bool isReadOnly, int textEntryBoxWidth, int textEntryBoxHeight)
{
    textBoxPosition = position;
    textBoxIsReadOnly = isReadOnly;
    textBoxWidth = textEntryBoxWidth;
    textBoxHeight = textEntryBoxHeight;

    resized();
    repaint();
}

Slider::TextEntryBoxPosition Slider::getTextBoxPosition() const
{
    return textBoxPosition;
}

bool Slider::isTextBoxReadOnly() const
{
    return textBoxIsReadOnly;
}

//==============================================================================

void Slider::setPopupDisplayEnabled (bool shouldShowBubble, Component* bubbleComponent)
{
    popupDisplayEnabled = shouldShowBubble;

    if (bubbleComponent != nullptr)
        popupBubbleComponent.reset (bubbleComponent);
}

void Slider::setPopupMenuEnabled (bool shouldShowMenu)
{
    popupMenuEnabled = shouldShowMenu;
}

//==============================================================================

bool Slider::isMouseOver() const
{
    return isMouseOverSlider;
}

bool Slider::isCurrentlyBeingDragged() const
{
    return dragMode != notDragging;
}

//==============================================================================

void Slider::setMouseDragSensitivity (double sensitivity)
{
    sensitivity = std::max (0.001, sensitivity);

    if (approximatelyEqual (mouseDragSensitivity, sensitivity))
        return;

    mouseDragSensitivity = sensitivity;
    notifyPropertyChanged (SliderIds::dragSensitivity);
}

double Slider::getMouseDragSensitivity() const
{
    return mouseDragSensitivity;
}

void Slider::setFineDragScale (double scale)
{
    scale = std::max (0.001, scale);

    if (approximatelyEqual (fineDragScale, scale))
        return;

    fineDragScale = scale;
    notifyPropertyChanged (SliderIds::fineDragScale);
}

double Slider::getFineDragScale() const
{
    return fineDragScale;
}

void Slider::setFineDragModifiers (KeyModifiers modifiers)
{
    if (fineDragModifiers == modifiers)
        return;

    fineDragModifiers = modifiers;
    notifyPropertyChanged (SliderIds::fineDragModifier);
}

KeyModifiers Slider::getFineDragModifiers() const
{
    return fineDragModifiers;
}

void Slider::setResetModifiers (KeyModifiers modifiers)
{
    if (resetModifiers == modifiers)
        return;

    resetModifiers = modifiers;
    notifyPropertyChanged (SliderIds::resetModifier);
}

KeyModifiers Slider::getResetModifiers() const
{
    return resetModifiers;
}

void Slider::setVelocityModeParameters (double sensitivity, double threshold, double offsetThreshold)
{
    velocitySensitivity = std::max (0.001, sensitivity);
    velocityThreshold = std::max (0.001, threshold);
    velocityOffsetThreshold = std::max (0.0, offsetThreshold);
}

//==============================================================================

void Slider::resized()
{
    auto bounds = getLocalBounds();

    if (textBoxPosition != NoTextBox)
    {
        auto textBounds = getTextBoxBounds();

        if (textEditor != nullptr)
            textEditor->setBounds (textBounds);
    }
}

void Slider::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================

void Slider::mouseEnter (const MouseEvent& event)
{
    isMouseOverSlider = true;
    pointerPosition = event.getPosition();
    repaint();
}

void Slider::mouseExit (const MouseEvent& event)
{
    isMouseOverSlider = false;
    repaint();
}

void Slider::mouseMove (const MouseEvent& event)
{
    const auto previousThumb = getHoveredThumb();

    pointerPosition = event.getPosition();

    if (getHoveredThumb() != previousThumb)
        repaint();
}

void Slider::mouseDown (const MouseEvent& event)
{
    // Check for right-click instead of isPopupTrigger()
    if (event.isRightButtonDown() && popupMenuEnabled)
    {
        showPopupMenu();
        return;
    }

    mouseDownPosition = event.getPosition(); // Already returns Point<float>, no toFloat() needed
    mouseDragStartPosition = mouseDownPosition;
    pointerPosition = mouseDownPosition;
    isMouseOverSlider = true;

    // A press on a button steps the value, and again while it is held
    for (const auto direction : { -1, 1 })
    {
        if (! getStepButtonBounds (direction).contains (mouseDownPosition))
            continue;

        const BailOutChecker bailOutChecker (this);

        pressedStep = direction;
        stepValue (direction);

        if (bailOutChecker.shouldBailOut())
            return;

        notifyPropertyChanged (SliderIds::pressedStep);

        if (bailOutChecker.shouldBailOut())
            return;

        stepRepeater.startTimer (stepRepeatDelayMs);

        if (getClickingGrabFocus())
            takeKeyboardFocus();

        repaint();
        return;
    }

    // A click with the reset modifier brings the default value back, and starts no drag
    if (areModifiersDown (resetModifiers, event.getModifiers()))
    {
        const BailOutChecker bailOutChecker (this);

        resetToDefaultValue();

        if (bailOutChecker.shouldBailOut())
            return;

        if (getClickingGrabFocus())
            takeKeyboardFocus();

        repaint();
        return;
    }

    const bool wasDragging = dragMode != notDragging;
    dragMode = getDragModeForMousePosition (mouseDownPosition);
    dragScale = getDragScale (event.getModifiers());

    if (dragMode != notDragging)
    {
        const BailOutChecker bailOutChecker (this);

        if (! wasDragging)
        {
            notifyPropertyChanged (SliderIds::dragging);
            if (bailOutChecker.shouldBailOut())
                return;
        }

        if (onDragStart)
        {
            onDragStart (event);
            if (bailOutChecker.shouldBailOut())
                return;
        }

        // A press on the track of a linear slider, beside its thumb, brings the thumb under the pointer
        if (isLinearSlider (sliderType) && ! getThumbBounds().contains (mouseDownPosition))
        {
            setValueNormalised (getNormalisedValueAt (mouseDownPosition));

            if (bailOutChecker.shouldBailOut())
                return;

            // The thumb stops at the ends of the track and on the steps of the range: the drag goes on from where it is
            mouseDragStartPosition = getThumbBounds().getCenter();
        }

        valueOnMouseDown = currentValue;
        minValueOnMouseDown = minValue;
        maxValueOnMouseDown = maxValue;
    }

    if (getClickingGrabFocus())
        takeKeyboardFocus();

    repaint();
}

void Slider::mouseUp (const MouseEvent& event)
{
    const BailOutChecker bailOutChecker (this);

    pointerPosition = event.getPosition();
    endInteraction (event);

    if (bailOutChecker.shouldBailOut())
        return;

    if (isMouseOverSlider && ! getLocalBounds().contains (event.getPosition()))
    {
        isMouseOverSlider = false;
        repaint();
    }
}

void Slider::mouseDrag (const MouseEvent& event)
{
    if (dragMode == notDragging)
        return;

    // The fine drag modifier pressed or released goes on from the value the slider has, at the speed it now asks for
    if (const auto scale = getDragScale (event.getModifiers()); ! exactlyEqual (scale, dragScale))
    {
        dragScale = scale;
        mouseDragStartPosition = event.getPosition();
        valueOnMouseDown = currentValue;
        minValueOnMouseDown = minValue;
        maxValueOnMouseDown = maxValue;
    }

    updateValueFromMousePosition (event.getPosition(), dragMode); // Already returns Point<float>
}

bool Slider::mouseWheel (const MouseEvent& event, const MouseWheelData& data)
{
    if (! isEnabled())
        return false;

    const auto previousValue = getValue();
    const double distance = (data.getDeltaX() + data.getDeltaY()) * 0.01 * getDragScale (event.getModifiers()) * mouseDragSensitivity;

    setValueNormalised (getValueNormalised() + distance);
    return getValue() != previousValue;
}

void Slider::mouseDoubleClick (const MouseEvent& event)
{
    if (! isEnabled())
        return;

    // Two quick presses on a button of a stepper are two steps, not a way back to the default value
    for (const auto direction : { -1, 1 })
        if (getStepButtonBounds (direction).contains (event.getPosition()))
            return;

    resetToDefaultValue();
}

//==============================================================================

bool Slider::keyDown (const KeyPress& key, const Point<float>& position)
{
    if (! isEnabled())
        return false;

    return updateValueFromKeypress (key);
}

void Slider::focusGained()
{
    repaint();
}

void Slider::focusLost()
{
    hideTextEditor (false);
    repaint();
}

void Slider::enablementChanged()
{
    if (! isEnabled())
        cancelInteraction();
}

void Slider::visibilityChanged()
{
    if (! isShowing())
        cancelInteraction();
}

//==============================================================================

void Slider::sendValueChanged (NotificationType notification)
{
    sendChangeNotification (notification, [this]
    {
        valueChanged();

        if (onValueChanged)
            onValueChanged (getValue());
    });
}

void Slider::sendMinValueChanged (NotificationType notification)
{
    sendChangeNotification (notification, [this]
    {
        minValueChanged();

        if (onMinValueChanged)
            onMinValueChanged (getMinValue());
    });
}

void Slider::sendMaxValueChanged (NotificationType notification)
{
    sendChangeNotification (notification, [this]
    {
        maxValueChanged();

        if (onMaxValueChanged)
            onMaxValueChanged (getMaxValue());
    });
}

void Slider::sendRangeChanged (const NormalisableRange<double>& previousRange)
{
    const BailOutChecker bailOutChecker (this);

    if (! approximatelyEqual (previousRange.start, range.start))
    {
        notifyPropertyChanged (SliderIds::minimum);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (! approximatelyEqual (previousRange.end, range.end))
    {
        notifyPropertyChanged (SliderIds::maximum);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (! approximatelyEqual (previousRange.interval, range.interval))
    {
        notifyPropertyChanged (SliderIds::interval);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (! approximatelyEqual (previousRange.skew, range.skew))
        notifyPropertyChanged (SliderIds::skew);
}

//==============================================================================

void Slider::updateValueFromMousePosition (Point<float> mousePos, DragMode dragMode)
{
    const auto sliderBounds = getTrackBounds();

    if (sliderBounds.isEmpty())
        return;

    float mouseDelta = 0.0f;
    float totalRange = 1.0f;

    // Calculate mouse delta based on slider type
    switch (sliderType)
    {
        case LinearVertical:
        case LinearBarVertical:
        case TwoValueVertical:
            mouseDelta = mouseDragStartPosition.getY() - mousePos.getY();
            totalRange = sliderBounds.getHeight();
            break;

        case LinearHorizontal:
        case LinearBarHorizontal:
            mouseDelta = mousePos.getX() - mouseDragStartPosition.getX();
            totalRange = sliderBounds.getWidth();
            break;

        case RotaryVerticalDrag:
        case IncDecButtons:
            mouseDelta = mouseDragStartPosition.getY() - mousePos.getY();
            totalRange = sliderBounds.getHeight();
            break;

        case RotaryHorizontalDrag:
            mouseDelta = mousePos.getX() - mouseDragStartPosition.getX();
            totalRange = sliderBounds.getWidth();
            break;

        case Rotary:
        {
            // For regular rotary, use both horizontal and vertical movement
            const float deltaX = mousePos.getX() - mouseDragStartPosition.getX();
            const float deltaY = mouseDragStartPosition.getY() - mousePos.getY();
            mouseDelta = deltaX + deltaY; // Combine both axes
            totalRange = std::max (sliderBounds.getWidth(), sliderBounds.getHeight());
            break;
        }

        default:
            // For other slider types, default to horizontal behavior
            mouseDelta = mousePos.getX() - mouseDragStartPosition.getX();
            totalRange = sliderBounds.getWidth();
            break;
    }

    if (totalRange <= 0.0f)
        return;

    const double normalizedDelta = (mouseDelta / totalRange) * mouseDragSensitivity * dragScale;

    switch (dragMode)
    {
        case draggingForValue:
        {
            const double newNormalisedValue = range.convertTo0to1 (valueOnMouseDown) + normalizedDelta;
            setValueNormalised (newNormalisedValue);
            break;
        }

        case draggingForMinValue:
        {
            const double newNormalisedValue = range.convertTo0to1 (minValueOnMouseDown) + normalizedDelta;
            setMinValue (jmin (range.convertFrom0to1 (std::clamp (newNormalisedValue, 0.0, 1.0)), maxValue));
            break;
        }

        case draggingForMaxValue:
        {
            const double newNormalisedValue = range.convertTo0to1 (maxValueOnMouseDown) + normalizedDelta;
            setMaxValue (jmax (range.convertFrom0to1 (std::clamp (newNormalisedValue, 0.0, 1.0)), minValue));
            break;
        }

        default:
            break;
    }
}

double Slider::getDragScale (const KeyModifiers& modifiers) const
{
    return areModifiersDown (fineDragModifiers, modifiers) ? fineDragScale : 1.0;
}

bool Slider::updateValueFromKeypress (const KeyPress& key)
{
    const bool isUpKey = key.getKey() == KeyPress::upKey || key.getKey() == KeyPress::rightKey;
    const bool isDownKey = key.getKey() == KeyPress::downKey || key.getKey() == KeyPress::leftKey;
    const bool isPageUp = key.getKey() == KeyPress::pageUpKey;
    const bool isPageDown = key.getKey() == KeyPress::pageDownKey;
    const bool isHome = key.getKey() == KeyPress::homeKey;
    const bool isEnd = key.getKey() == KeyPress::endKey;

    if (! (isUpKey || isDownKey || isPageUp || isPageDown || isHome || isEnd))
        return false;

    const double currentNormalisedValue = getValueNormalised();
    double newNormalisedValue = currentNormalisedValue;

    if (isHome)
        newNormalisedValue = 0.0;
    else if (isEnd)
        newNormalisedValue = 1.0;
    else
    {
        const double increment = (isPageUp || isPageDown) ? 0.1 : 0.01;
        const double direction = (isUpKey || isPageUp) ? 1.0 : -1.0;
        newNormalisedValue = currentNormalisedValue + (direction * increment);
    }

    setValueNormalised (newNormalisedValue);
    return true;
}

void Slider::resetToDefaultValue()
{
    setValue (defaultValue);
}

//==============================================================================

double Slider::constrainValue (double valueToConstrain) const
{
    return snapToLegalValue (range.getRange().clipValue (valueToConstrain));
}

double Slider::snapToLegalValue (double valueToSnap) const
{
    return range.snapToLegalValue (valueToSnap);
}

//==============================================================================

Rectangle<float> Slider::getTrackBounds() const
{
    auto bounds = getLocalBounds();

    if (textBoxPosition != NoTextBox)
    {
        const auto textBounds = getTextBoxBounds();

        switch (textBoxPosition)
        {
            case TextBoxLeft:
                bounds.removeFromLeft (textBounds.getWidth());
                break;
            case TextBoxRight:
                bounds.removeFromRight (textBounds.getWidth());
                break;
            case TextBoxAbove:
                bounds.removeFromTop (textBounds.getHeight());
                break;
            case TextBoxBelow:
                bounds.removeFromBottom (textBounds.getHeight());
                break;
            default:
                break;
        }
    }

    // Add margin and account for thumb size to prevent clipping
    float margin = 2.0f;

    // For linear sliders, add extra margin based on thumb size to prevent clipping: the thumb is drawn around both ends
    // of the track, as large as it is styled
    switch (sliderType)
    {
        case LinearHorizontal:
        case LinearBarHorizontal:
        case TwoValueHorizontal:
        {
            const float thumbSize = jmax (std::min (20.0f, bounds.getHeight() * 0.8f), findPropertyAs<float> (SliderIds::thumbSize));
            const float thumbMargin = thumbSize * 0.5f;
            return Rectangle<float> (bounds.getX() + thumbMargin, bounds.getY() + margin, bounds.getWidth() - (thumbMargin * 2), bounds.getHeight() - (margin * 2));
        }

        case LinearVertical:
        case LinearBarVertical:
        case TwoValueVertical:
        {
            const float thumbSize = jmax (std::min (20.0f, bounds.getWidth() * 0.8f), findPropertyAs<float> (SliderIds::thumbSize));
            const float thumbMargin = thumbSize * 0.5f;
            return Rectangle<float> (bounds.getX() + margin, bounds.getY() + thumbMargin, bounds.getWidth() - (margin * 2), bounds.getHeight() - (thumbMargin * 2));
        }

        case IncDecButtons:
        {
            // A stepper keeps the thickness of a button in whatever room it is given, and stays in the middle of it:
            // on a line in a wide area, its buttons one above the other in a narrow one
            const auto area = Rectangle<float> (bounds.getX() + margin, bounds.getY() + margin, bounds.getWidth() - (margin * 2), bounds.getHeight() - (margin * 2));
            const float thickness = jmax (20.0f, findPropertyAs<float> (SliderIds::thumbSize) * 1.75f);

            return area.getWidth() >= area.getHeight()
                     ? area.withSizeKeepingCenter (area.getWidth(), jmin (area.getHeight(), thickness))
                     : area.withSizeKeepingCenter (jmin (area.getWidth(), thickness), area.getHeight());
        }

        default:
            // For rotary and other types, use smaller margin
            return Rectangle<float> (bounds.getX() + margin, bounds.getY() + margin, bounds.getWidth() - (margin * 2), bounds.getHeight() - (margin * 2));
    }
}

Rectangle<float> Slider::getStepButtonBounds (int direction) const
{
    if (sliderType != IncDecButtons)
        return {};

    auto bounds = getTrackBounds();

    // On a line, the value rises to the right. One above the other, it rises upward
    if (bounds.getWidth() >= bounds.getHeight())
    {
        const auto size = jmin (bounds.getHeight(), bounds.getWidth() / 3.0f);
        return direction > 0 ? bounds.removeFromRight (size) : bounds.removeFromLeft (size);
    }

    const auto size = jmin (bounds.getWidth(), bounds.getHeight() / 3.0f);
    return direction > 0 ? bounds.removeFromTop (size) : bounds.removeFromBottom (size);
}

Rectangle<float> Slider::getDecrementBounds() const
{
    return getStepButtonBounds (-1);
}

Rectangle<float> Slider::getIncrementBounds() const
{
    return getStepButtonBounds (1);
}

Rectangle<float> Slider::getTextBounds() const
{
    if (sliderType != IncDecButtons)
        return {};

    const auto track = getTrackBounds();
    const auto decrement = getDecrementBounds();
    const auto increment = getIncrementBounds();

    return approximatelyEqual (decrement.getY(), increment.getY())
             ? Rectangle<float> (decrement.getRight(), track.getY(), increment.getX() - decrement.getRight(), track.getHeight())
             : Rectangle<float> (track.getX(), increment.getBottom(), track.getWidth(), decrement.getY() - increment.getBottom());
}

void Slider::stepValue (int direction)
{
    const auto interval = getInterval();
    const auto amount = interval > 0.0 ? interval : (range.end - range.start) / 100.0;

    setValue (currentValue + direction * amount);
}

void Slider::repeatStep()
{
    if (pressedStep == 0)
    {
        stepRepeater.stopTimer();
        return;
    }

    stepRepeater.startTimer (stepRepeatIntervalMs);
    stepValue (pressedStep);
}

Rectangle<float> Slider::getTextBoxBounds() const
{
    if (textBoxPosition == NoTextBox)
        return {};

    auto bounds = getLocalBounds();

    switch (textBoxPosition)
    {
        case TextBoxLeft:
            return bounds.removeFromLeft (textBoxWidth);
        case TextBoxRight:
            return bounds.removeFromRight (textBoxWidth);
        case TextBoxAbove:
            return bounds.removeFromTop (textBoxHeight);
        case TextBoxBelow:
            return bounds.removeFromBottom (textBoxHeight);
        default:
            return {};
    }
}

//==============================================================================

Rectangle<float> Slider::getThumbBoundsAt (double normalisedValue) const
{
    const auto track = getTrackBounds();
    const auto size = findPropertyAs<float> (SliderIds::thumbSize);
    const auto proportion = static_cast<float> (normalisedValue);

    const auto center = isVerticalSlider (sliderType)
                          ? Point<float> (track.getCenterX(), track.getBottom() - proportion * track.getHeight())
                          : Point<float> (track.getX() + proportion * track.getWidth(), track.getCenterY());

    return Rectangle<float> (center.getX() - size * 0.5f, center.getY() - size * 0.5f, size, size);
}

Rectangle<float> Slider::getThumbBounds() const
{
    return isLinearSlider (sliderType) ? getThumbBoundsAt (getValueNormalised()) : Rectangle<float>();
}

Rectangle<float> Slider::getMinThumbBounds() const
{
    return isTwoValueSlider (sliderType) ? getThumbBoundsAt (getMinValueNormalised()) : Rectangle<float>();
}

Rectangle<float> Slider::getMaxThumbBounds() const
{
    return isTwoValueSlider (sliderType) ? getThumbBoundsAt (getMaxValueNormalised()) : Rectangle<float>();
}

Slider::Thumb Slider::getHoveredThumb() const
{
    switch (dragMode)
    {
        case draggingForMinValue:
            return Thumb::minValue;

        case draggingForMaxValue:
            return Thumb::maxValue;

        case draggingForValue:
            return isLinearSlider (sliderType) ? Thumb::value : Thumb::none;

        default:
            break;
    }

    if (! isMouseOverSlider)
        return Thumb::none;

    // The thumb of the upper value is painted over the other one
    if (isTwoValueSlider (sliderType))
    {
        if (getMaxThumbBounds().contains (pointerPosition))
            return Thumb::maxValue;

        return getMinThumbBounds().contains (pointerPosition) ? Thumb::minValue : Thumb::none;
    }

    return isLinearSlider (sliderType) && getThumbBounds().contains (pointerPosition) ? Thumb::value : Thumb::none;
}

double Slider::getNormalisedValueAt (Point<float> position) const
{
    const auto track = getTrackBounds();
    const auto isVertical = isVerticalSlider (sliderType);
    const auto length = isVertical ? track.getHeight() : track.getWidth();

    // A track with no length has no position to tell: the value stays where it is
    if (length <= 0.0f)
        return getValueNormalised();

    const auto distance = isVertical ? track.getBottom() - position.getY() : position.getX() - track.getX();

    return std::clamp (static_cast<double> (distance / length), 0.0, 1.0);
}

void Slider::endInteraction (const MouseEvent& event)
{
    const BailOutChecker bailOutChecker (this);

    if (pressedStep != 0)
    {
        pressedStep = 0;
        stepRepeater.stopTimer();
        repaint();

        notifyPropertyChanged (SliderIds::pressedStep);

        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (dragMode == notDragging)
        return;

    dragMode = notDragging;
    repaint();

    notifyPropertyChanged (SliderIds::dragging);

    if (bailOutChecker.shouldBailOut())
        return;

    if (onDragEnd)
        onDragEnd (event);
}

void Slider::cancelInteraction()
{
    endInteraction (MouseEvent (MouseEvent::noButtons, KeyModifiers(), pointerPosition, this));
}

Slider::DragMode Slider::getDragModeForMousePosition (Point<float> mousePos) const
{
    switch (sliderType)
    {
        case LinearHorizontal:
        case LinearBarHorizontal:
        case LinearVertical:
        case LinearBarVertical:
        {
            // The thumb is drawn around the ends of the track: a press on what of it is outside the track grabs it too
            return isMouseOverSliderArea (mousePos) || getThumbBounds().contains (mousePos) ? draggingForValue : notDragging;
        }

        case TwoValueHorizontal:
        case TwoValueVertical:
        {
            // Determine which thumb is closer to the mouse position
            const auto sliderBounds = getTrackBounds();
            const bool isHorizontal = (sliderType == TwoValueHorizontal);

            const float minPos = isHorizontal
                                   ? sliderBounds.getX() + (range.convertTo0to1 (minValue) * sliderBounds.getWidth())
                                   : sliderBounds.getBottom() - (range.convertTo0to1 (minValue) * sliderBounds.getHeight());

            const float maxPos = isHorizontal
                                   ? sliderBounds.getX() + (range.convertTo0to1 (maxValue) * sliderBounds.getWidth())
                                   : sliderBounds.getBottom() - (range.convertTo0to1 (maxValue) * sliderBounds.getHeight());

            const float mouseCoord = isHorizontal ? mousePos.getX() : mousePos.getY(); // Use getX() and getY()
            const float distToMin = std::abs (mouseCoord - minPos);
            const float distToMax = std::abs (mouseCoord - maxPos);

            return (distToMin < distToMax) ? draggingForMinValue : draggingForMaxValue;
        }

        default:
            // For rotary and other slider types, use existing behavior
            return isMouseOverSliderArea (mousePos) ? draggingForValue : notDragging;
    }
}

bool Slider::isMouseOverSliderArea (Point<float> mousePos) const
{
    const auto sliderBounds = getTrackBounds();
    return sliderBounds.contains (mousePos);
}

//==============================================================================

void Slider::showPopupMenu()
{
    // TODO: Implement popup menu with reset to default, etc.
}

void Slider::createTextEditor()
{
    // TODO: Implement text editor creation
}

void Slider::hideTextEditor (bool discardCurrentEditorContents)
{
    if (textEditor != nullptr)
    {
        if (! discardCurrentEditorContents)
        {
            // TODO: Parse text and update value
        }

        textEditor.reset();
    }
}

} // namespace yup
