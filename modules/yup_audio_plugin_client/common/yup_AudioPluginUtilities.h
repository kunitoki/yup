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

//==============================================================================
namespace yup
{

/** Returns the first host-facing parameter ID not used by the processor.

    This is useful for wrapper-owned synthetic parameters, such as bypass, that
    must not collide with plugin-authored stable automation IDs.
*/
inline uint32 findFirstUnusedHostParameterID (const AudioProcessor& processor, uint32 preferredParameterID)
{
    auto parameterID = preferredParameterID;

    while (processor.getParameterByHostID (parameterID) != nullptr
           && parameterID < AudioParameter::maximumHostParameterID)
    {
        ++parameterID;
    }

    jassert (parameterID <= AudioParameter::maximumHostParameterID);
    jassert (processor.getParameterByHostID (parameterID) == nullptr);

    return parameterID;
}

/** Returns a host-facing parameter ID suitable for a wrapper-owned bypass parameter. */
inline uint32 getBypassHostParameterID (const AudioProcessor& processor)
{
    return findFirstUnusedHostParameterID (processor, static_cast<uint32> (processor.getParameters().size()));
}

/** Adds a normalized automation change for a host-facing parameter ID.

    @returns true if the host parameter ID mapped to a processor parameter and
             the change was added to the buffer.
*/
inline bool addParameterChangeByHostParameterID (AudioProcessor& processor,
                                                 ParameterChangeBuffer& changes,
                                                 uint32 hostParameterID,
                                                 float normalizedValue,
                                                 int sampleOffset)
{
    const auto parameters = processor.getParameters();
    const auto parameterIndex = processor.getParameterIndexByHostID (hostParameterID);

    if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
        return false;

    return changes.addChange (parameterIndex, normalizedValue, sampleOffset);
}

/** Applies the last known normalized values in a parameter change buffer.

    Wrappers use this after collecting sample-accurate changes so processors that
    still read AudioParameter atomics directly see the latest host value.
*/
inline void applyParameterChangesToProcessor (AudioProcessor& processor, const ParameterChangeBuffer& changes)
{
    const auto parameters = processor.getParameters();

    for (const auto& change : changes)
    {
        if (isPositiveAndBelow (change.parameterIndex, static_cast<int> (parameters.size())))
            parameters[change.parameterIndex]->setNormalizedValue (change.normalizedValue);
    }
}

/** Ends any active parameter gestures before a plugin wrapper tears down its processor. */
inline void endActiveParameterGestures (AudioProcessor* processor)
{
    if (processor == nullptr)
        return;

    for (auto& parameter : processor->getParameters())
    {
        while (parameter->isPerformingChangeGesture())
            parameter->endChangeGesture();
    }
}

} // namespace yup
