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

class BindingModelTests : public ::testing::Test
{
protected:
    class MockGestureProvider final : public BindingProvider
    {
    public:
        std::vector<Property> getProperties() const override
        {
            return {
                { Identifier ("cutoff"), ComponentProperty::Type::number },
                { Identifier ("muted"), ComponentProperty::Type::boolean }
            };
        }

        var getValue (const Identifier& name) const override
        {
            if (const auto* value = values.getVarPointer (name))
                return *value;

            return {};
        }

        void setValue (const Identifier& name, const var& value) override
        {
            if (values.set (name, value))
                notifyValueChanged (name);
        }

        void beginGesture (const Identifier& name) override
        {
            begunGestures.add (name.toString());
        }

        void endGesture (const Identifier& name) override
        {
            endedGestures.add (name.toString());
        }

        NamedValueSet values { { Identifier ("cutoff"), 0.5 }, { Identifier ("muted"), false } };
        StringArray begunGestures;
        StringArray endedGestures;
    };

    class PressableControl final : public Component
    {
    public:
        const ComponentTypeInfo& getDefaultTypeInfo() const override
        {
            static const ComponentTypeInfo info = ComponentTypeBuilder<PressableControl> ("PressableControl")
                .state ("pressed", &PressableControl::isPressed, &PressableControl::setPressed, false)
                .build();
            return info;
        }

        bool isPressed() const noexcept { return pressedState; }

        void setPressed (bool shouldBePressed)
        {
            if (pressedState == shouldBePressed)
                return;

            pressedState = shouldBePressed;
            notifyPropertyChanged (Identifier ("pressed"));
        }

    private:
        bool pressedState = false;
    };
};

TEST_F (BindingModelTests, MirrorsDynamicDataTreePropertiesWithoutComponentBaseProperties)
{
    DataTree state ("params");
    state.beginTransaction().setProperty ("gain", 0.75);
    DataTreeBindingProvider provider (state);
    BindingModel model (provider);

    EXPECT_EQ (model.getProvider(), &provider);
    EXPECT_TRUE (model.getTypeInfo().hasProperty (Identifier ("gain")));
    EXPECT_FALSE (model.getTypeInfo().hasProperty (Identifier ("opacity")));
    EXPECT_DOUBLE_EQ (static_cast<double> (model.getProperty (Identifier ("gain"))), 0.75);

    // Writing through BindingModel updates the provider; adding a new key dynamically refreshes type info
    model.setProperty (Identifier ("gain"), 0.25);
    EXPECT_DOUBLE_EQ (static_cast<double> (state.getProperty ("gain")), 0.25);

    state.beginTransaction().setProperty ("drive", 3.0);
    EXPECT_TRUE (model.getTypeInfo().hasProperty (Identifier ("drive")));
    EXPECT_DOUBLE_EQ (static_cast<double> (model.getProperty (Identifier ("drive"))), 3.0);
}

TEST_F (BindingModelTests, TracksControlPressGesturesAndDetachesWhenProviderOrControlIsDestroyed)
{
    auto provider = std::make_unique<MockGestureProvider>();
    BindingModel model (*provider);

    {
        PressableControl knob;
        model.trackGestures (knob, Identifier ("cutoff"));

        knob.setPressed (true);
        EXPECT_EQ (provider->begunGestures, StringArray ({ "cutoff" }));
        EXPECT_TRUE (provider->endedGestures.isEmpty());

        // Destroying a tracked control mid-gesture ends the active gesture cleanly
    }

    EXPECT_EQ (provider->endedGestures, StringArray ({ "cutoff" }));

    provider.reset();
    EXPECT_EQ (model.getProvider(), nullptr);
}
