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

    /** A provider exposing the properties the test lists, whatever their values are. */
    class MockDynamicProvider final : public BindingProvider
    {
    public:
        std::vector<Property> getProperties() const override
        {
            return properties;
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

        std::vector<Property> properties {
            { Identifier ("payload"), ComponentProperty::Type::any },
            { Identifier ("gain"), ComponentProperty::Type::number }
        };

        NamedValueSet values;
    };

    /** A provider listing nothing: its values are found by their name only, as those of a provider with many of
        them, or making them when they are first asked for, are. */
    class FindByNameProvider final : public BindingProvider
    {
    public:
        std::vector<Property> getProperties() const override { return {}; }

        std::optional<Property> findProperty (const Identifier& name) const override
        {
            if (values.contains (name))
                return Property { name, ComponentProperty::Type::number };

            return std::nullopt;
        }

        var getValue (const Identifier& name) const override { return values.getWithDefault (name, {}); }

        void setValue (const Identifier& name, const var& value) override
        {
            if (values.set (name, value))
                notifyValueChanged (name);
        }

        NamedValueSet values { { Identifier ("tempo"), 120.0 } };
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

TEST_F (BindingModelTests, KeepsItsTypeInfoWhileTheProviderExposesTheSameProperties)
{
    struct Listener : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            changed.add (name.toString());
        }

        StringArray changed;
    };

    Listener listener;
    MockDynamicProvider provider;
    BindingModel model (provider);
    model.addComponentListener (&listener);

    const auto* info = &model.getTypeInfo();
    const auto* payload = info->findProperty (Identifier ("payload"));
    ASSERT_NE (payload, nullptr);
    EXPECT_EQ (payload->type, ComponentProperty::Type::any);

    // An untyped property takes values of any kind, and the properties stay the ones that were read
    provider.setValue (Identifier ("payload"), 1.0);
    provider.setValue (Identifier ("payload"), "text");

    EXPECT_EQ (&model.getTypeInfo(), info);
    EXPECT_EQ (model.getTypeInfo().findProperty (Identifier ("payload")), payload);
    EXPECT_EQ (model.getProperty (Identifier ("payload")).toString(), "text");
    EXPECT_EQ (listener.changed, StringArray ({ "payload", "payload" }));

    // They stay as well when they are asked for with nothing that differs
    model.refreshProperties();
    EXPECT_EQ (&model.getTypeInfo(), info);
}

TEST_F (BindingModelTests, MakesItsTypeInfoAgainWhenThePropertiesOrTheirTypesDiffer)
{
    MockDynamicProvider provider;
    BindingModel model (provider);

    // An untyped property that is given a type, told by one of its values changing
    auto* info = &model.getTypeInfo();
    provider.properties[0].type = ComponentProperty::Type::string;
    provider.setValue (Identifier ("payload"), "text");

    EXPECT_NE (&model.getTypeInfo(), info);
    ASSERT_NE (model.getTypeInfo().findProperty (Identifier ("payload")), nullptr);
    EXPECT_EQ (model.getTypeInfo().findProperty (Identifier ("payload"))->type, ComponentProperty::Type::string);

    // The type of the elements of a list
    info = &model.getTypeInfo();
    provider.properties[1] = { Identifier ("gain"), ComponentProperty::Type::array, ComponentProperty::Type::number };
    model.refreshProperties();

    EXPECT_NE (&model.getTypeInfo(), info);
    ASSERT_NE (model.getTypeInfo().findProperty (Identifier ("gain")), nullptr);
    EXPECT_EQ (model.getTypeInfo().findProperty (Identifier ("gain"))->elementType, ComponentProperty::Type::number);

    // A property that appears, told by its first value
    info = &model.getTypeInfo();
    provider.properties.push_back ({ Identifier ("drive"), ComponentProperty::Type::number });
    provider.setValue (Identifier ("drive"), 3.0);

    EXPECT_NE (&model.getTypeInfo(), info);
    EXPECT_TRUE (model.getTypeInfo().hasProperty (Identifier ("drive")));
    EXPECT_DOUBLE_EQ (static_cast<double> (model.getProperty (Identifier ("drive"))), 3.0);

    // A property that goes
    info = &model.getTypeInfo();
    provider.properties.erase (provider.properties.begin());
    model.refreshProperties();

    EXPECT_NE (&model.getTypeInfo(), info);
    EXPECT_FALSE (model.getTypeInfo().hasProperty (Identifier ("payload")));
    EXPECT_TRUE (model.getTypeInfo().hasProperty (Identifier ("gain")));

    // The same properties in another order
    info = &model.getTypeInfo();
    std::swap (provider.properties[0], provider.properties[1]);
    model.refreshProperties();

    EXPECT_NE (&model.getTypeInfo(), info);
}

TEST_F (BindingModelTests, AsksTheProviderForAPropertyItDoesNotList)
{
    FindByNameProvider provider;
    BindingModel model (provider);

    EXPECT_FALSE (model.getTypeInfo().hasProperty (Identifier ("tempo")));
    EXPECT_FALSE (model.ensureProperty (Identifier ("missing")));
    EXPECT_FALSE (model.getTypeInfo().hasProperty (Identifier ("missing")));

    EXPECT_TRUE (model.ensureProperty (Identifier ("tempo")));
    ASSERT_NE (model.getTypeInfo().findProperty (Identifier ("tempo")), nullptr);
    EXPECT_EQ (model.getTypeInfo().findProperty (Identifier ("tempo"))->type, ComponentProperty::Type::number);
    EXPECT_DOUBLE_EQ (static_cast<double> (model.getProperty (Identifier ("tempo"))), 120.0);

    // It is kept with the ones the provider lists, and the type info with it
    const auto* info = &model.getTypeInfo();
    model.refreshProperties();
    EXPECT_TRUE (model.ensureProperty (Identifier ("tempo")));
    EXPECT_EQ (&model.getTypeInfo(), info);
}
