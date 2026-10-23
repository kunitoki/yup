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
#include "ParameterGesture.h"
#include "SynthUndoHistory.h"

//==============================================================================
ParameterGesture::ParameterGesture (yup::Component& owningControl, yup::AudioParameter::Ptr parameterToMove)
    : owner (owningControl)
    , parameter (std::move (parameterToMove))
{
}

ParameterGesture::~ParameterGesture()
{
    end();
}

void ParameterGesture::setParameter (yup::AudioParameter::Ptr newParameter)
{
    end();
    parameter = std::move (newParameter);
}

void ParameterGesture::begin()
{
    if (active || parameter == nullptr)
        return;

    parameter->beginChangeGesture();
    startValue = parameter->getNormalizedValue();
    active = true;
}

void ParameterGesture::end()
{
    if (! active)
        return;

    active = false;
    parameter->endChangeGesture();
    record (startValue);
}

void ParameterGesture::setNormalized (float normalizedValue, bool joinsLastStep)
{
    if (parameter == nullptr)
        return;

    const auto value = parameter->convertToDenormalizedValue (yup::jlimit (0.0f, 1.0f, normalizedValue));

    if (active)
    {
        parameter->setValueNotifyingHost (value);
        return;
    }

    const auto valueBefore = parameter->getNormalizedValue();

    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (value);
    parameter->endChangeGesture();
    record (valueBefore, joinsLastStep);
}

void ParameterGesture::resetToDefault()
{
    if (parameter == nullptr)
        return;

    end();
    setNormalized (parameter->convertToNormalizedValue (parameter->getDefaultValue()));
}

/** Keeps a gesture that changed the value as one step of the undo history. */
void ParameterGesture::record (float valueBefore, bool joinsLastStep)
{
    const auto valueAfter = parameter->getNormalizedValue();

    if (yup::approximatelyEqual (valueBefore, valueAfter))
        return;

    SynthUndoHistory::record<float> (owner, valueBefore, valueAfter, [parameter = parameter] (float value)
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertToDenormalizedValue (value));
        parameter->endChangeGesture();
    }, joinsLastStep);
}
