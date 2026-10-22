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

#include "SynthCurveState.h"

namespace
{
const yup::Identifier envelopeType ("Envelope");
const yup::Identifier lfoType ("LFO");
const yup::Identifier indexProperty ("index");
const yup::Identifier pointsProperty ("points");
const yup::Identifier sustainProperty ("sustain");
const yup::Identifier loopStartProperty ("loopStart");
const yup::Identifier loopEndProperty ("loopEnd");

yup::DataTree createCurveTree (const yup::Identifier& type, int index, const SynthCurveState::Curve& curve)
{
    yup::StringArray points;

    for (int point = 0; point < curve.getNumPoints(); ++point)
    {
        const auto& p = curve.getPoint (point);
        points.add (yup::String (p.time, 6) + " " + yup::String (p.level, 6) + " " + yup::String (p.curve, 6));
    }

    yup::DataTree tree (type);
    auto transaction = tree.beginTransaction();
    transaction.setProperty (indexProperty, index);
    transaction.setProperty (pointsProperty, points.joinIntoString (";"));
    transaction.setProperty (sustainProperty, curve.getSustainIndex());
    transaction.setProperty (loopStartProperty, curve.getLoopStart());
    transaction.setProperty (loopEndProperty, curve.getLoopEnd());

    return tree;
}

/** Returns false when the tree holds no usable curve. */
bool readCurveTree (const yup::DataTree& tree, SynthCurveState::Curve& curve)
{
    const auto points = yup::StringArray::fromTokens (tree.getProperty (pointsProperty, {}).toString(), ";", "");

    SynthCurveState::Curve result;

    for (const auto& point : points)
    {
        const auto fields = yup::StringArray::fromTokens (point, " ", "");

        if (fields.size() == 3)
            result.addPoint (fields[0].getFloatValue(), fields[1].getFloatValue(), fields[2].getFloatValue());
    }

    if (result.getNumPoints() < 2)
        return false;

    result.setSustainIndex (static_cast<int> (tree.getProperty (sustainProperty, -1)));
    result.setLoop (static_cast<int> (tree.getProperty (loopStartProperty, -1)), static_cast<int> (tree.getProperty (loopEndProperty, -1)));

    curve = result;
    return true;
}
} // namespace

const yup::Identifier SynthCurveState::treeType ("Curves");

SynthCurveState::SynthCurveState()
{
    for (std::size_t index = 0; index < envelopes.size(); ++index)
        envelopes[index] = makeDefaultEnvelope (static_cast<int> (index));

    for (auto& curve : lfos)
        curve = makeDefaultLFO();
}

//==============================================================================
SynthCurveState::Curve SynthCurveState::getEnvelope (int index) const
{
    const yup::SpinLock::ScopedLockType scopedLock (lock);
    return envelopes[static_cast<std::size_t> (yup::jlimit (0, SynthExample::envelopeCount - 1, index))];
}

void SynthCurveState::setEnvelope (int index, const Curve& curve)
{
    if (! yup::isPositiveAndBelow (index, SynthExample::envelopeCount))
        return;

    {
        const yup::SpinLock::ScopedLockType scopedLock (lock);
        envelopes[static_cast<std::size_t> (index)] = index == 0 ? sanitizeAmplitude (curve) : curve;
    }

    generation.fetch_add (1);
}

SynthCurveState::Curve SynthCurveState::getLFO (int index) const
{
    const yup::SpinLock::ScopedLockType scopedLock (lock);
    return lfos[static_cast<std::size_t> (yup::jlimit (0, SynthExample::lfoCount - 1, index))];
}

void SynthCurveState::setLFO (int index, const Curve& curve)
{
    if (! yup::isPositiveAndBelow (index, SynthExample::lfoCount))
        return;

    {
        const yup::SpinLock::ScopedLockType scopedLock (lock);
        lfos[static_cast<std::size_t> (index)] = curve;
    }

    generation.fetch_add (1);
}

bool SynthCurveState::copyIfChanged (EnvelopeCurves& envelopesOut, LFOCurves& lfosOut, int& seenGeneration) const noexcept
{
    const auto current = generation.load();

    if (current == seenGeneration)
        return false;

    const yup::SpinLock::ScopedTryLockType scopedLock (lock);

    if (! scopedLock.isLocked())
        return false;

    envelopesOut = envelopes;
    lfosOut = lfos;
    seenGeneration = current;
    return true;
}

//==============================================================================
yup::DataTree SynthCurveState::createTree() const
{
    yup::DataTree tree (treeType);
    auto transaction = tree.beginTransaction();

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
        transaction.addChild (createCurveTree (envelopeType, index, getEnvelope (index)));

    for (int index = 0; index < SynthExample::lfoCount; ++index)
        transaction.addChild (createCurveTree (lfoType, index, getLFO (index)));

    return tree;
}

void SynthCurveState::readFrom (const yup::DataTree& tree)
{
    EnvelopeCurves newEnvelopes;
    LFOCurves newLFOs;

    for (std::size_t index = 0; index < newEnvelopes.size(); ++index)
        newEnvelopes[index] = makeDefaultEnvelope (static_cast<int> (index));

    for (auto& curve : newLFOs)
        curve = makeDefaultLFO();

    if (tree.isValid() && tree.getType() == treeType)
    {
        for (int child = 0; child < tree.getNumChildren(); ++child)
        {
            const auto curveTree = tree.getChild (child);
            const auto index = static_cast<int> (curveTree.getProperty (indexProperty, -1));

            if (curveTree.getType() == envelopeType && yup::isPositiveAndBelow (index, SynthExample::envelopeCount))
                readCurveTree (curveTree, newEnvelopes[static_cast<std::size_t> (index)]);
            else if (curveTree.getType() == lfoType && yup::isPositiveAndBelow (index, SynthExample::lfoCount))
                readCurveTree (curveTree, newLFOs[static_cast<std::size_t> (index)]);
        }
    }

    newEnvelopes[0] = sanitizeAmplitude (newEnvelopes[0]);

    {
        const yup::SpinLock::ScopedLockType scopedLock (lock);
        envelopes = newEnvelopes;
        lfos = newLFOs;
    }

    generation.fetch_add (1);
}

//==============================================================================
SynthCurveState::Curve SynthCurveState::makeAdsr (float attack, float decay, float sustain, float release, float curvature)
{
    Curve curve;
    curve.addPoint (0.0f, 0.0f);
    curve.addPoint (attack, 1.0f);
    curve.addPoint (attack + decay, sustain, curvature);
    curve.addPoint (attack + decay + release, 0.0f, curvature);
    curve.setSustainIndex (2);
    return curve;
}

SynthCurveState::Curve SynthCurveState::makeDefaultEnvelope (int index)
{
    if (index == 0)
        return makeAdsr (0.005f, 0.35f, 0.7f, 0.35f, -0.4f);

    return makeAdsr (0.2f, 0.5f, 0.3f, 0.5f, -0.4f);
}

SynthCurveState::Curve SynthCurveState::makeDefaultLFO()
{
    Curve curve;
    curve.addPoint (0.0f, 0.0f);
    curve.addPoint (0.5f, 1.0f);
    curve.addPoint (1.0f, 0.0f);
    return curve;
}

SynthCurveState::Curve SynthCurveState::sanitizeAmplitude (Curve curve)
{
    if (curve.getNumPoints() < 2)
        return makeDefaultEnvelope (0);

    const auto last = curve.getNumPoints() - 1;
    auto point = curve.getPoint (last);
    point.level = 0.0f;
    curve.setPoint (last, point);

    if (curve.getSustainIndex() == last)
        curve.setSustainIndex (-1);

    if (curve.getLoopEnd() == last)
        curve.clearLoop();

    return curve;
}
