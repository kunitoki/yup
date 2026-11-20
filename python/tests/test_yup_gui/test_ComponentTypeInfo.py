import gc

import yup

"""
The reflected component model: the ComponentTypeInfo of a component, the ComponentProperty and
ComponentSignal descriptors it lists, and the ComponentTypeRegistry the built-in types are in.

Type infos are static tables, and the components created through their factory are never laid
out or painted, so nothing here needs an application.

Not covered: ComponentTypeRegistry.registerType is not bound (the registry keeps a pointer to a
type info that has to outlive it), and neither are the getter, setter and connect functions of the
descriptors, which Component.getProperty, Component.setProperty and the widget callbacks stand for.
"""

BUILT_IN_TYPES = ["Component", "Button", "Layout", "Label", "Slider", "TextButton", "ToggleButton",
                  "SwitchButton", "ComboBox", "TextEditor", "ProgressBar", "ScrollBar",
                  "ResizableDivider", "ListBox", "Viewport", "TabComponent", "FlexBox", "Row",
                  "Column", "Grid", "Stack"]


def slider_info():
    return yup.Slider(yup.SliderType.LinearHorizontal).getTypeInfo()


def names_of(descriptors):
    return [str(descriptor.name) for descriptor in descriptors]


def registry():
    yup.registerBuiltInComponentTypes()

    return yup.ComponentTypeRegistry.getInstance()


# ==============================================================================
# Type info
# ==============================================================================

def test_type_info_names_the_type_and_its_base_chain():
    info = slider_info()

    assert str(info.getTypeName()) == "Slider"

    base = info.getBaseTypeInfo()
    assert base is not None
    assert str(base.getTypeName()) == "Component"
    assert base.getBaseTypeInfo() is None


def test_type_info_is_shared_by_the_components_of_a_type():
    # The info belongs to the widget class, so it outlives the components it was read from.
    first = yup.Slider(yup.SliderType.LinearHorizontal).getTypeInfo()
    second = yup.Slider(yup.SliderType.Rotary).getTypeInfo()

    assert first is second
    assert str(first.getTypeName()) == "Slider"


def test_type_info_knows_what_it_derives_from():
    info = slider_info()
    component_info = yup.Component().getTypeInfo()

    assert info.isSubtypeOf("Slider")
    assert info.isSubtypeOf("Component")
    assert info.isSubtypeOf(yup.Identifier("Component"))
    assert info.isSubtypeOf(component_info)
    assert not info.isSubtypeOf("Label")
    assert not component_info.isSubtypeOf(info)


def test_type_info_matches_the_existing_introspection():
    slider = yup.Slider(yup.SliderType.LinearHorizontal)
    info = slider.getTypeInfo()

    assert slider.getTypeName() == str(info.getTypeName())
    assert sorted(slider.getPropertyNames()) == sorted(names_of(info.getAllProperties()))


# ==============================================================================
# Properties
# ==============================================================================

def test_state_backed_property_is_described():
    value = slider_info().findProperty("value")

    assert value is not None
    assert str(value.name) == "value"
    assert value.type == yup.ComponentProperty.Type.number
    assert value.isStateBacked()
    assert not value.isStoreBacked()
    assert not value.isReadOnly()
    assert value.isState is False
    assert value.isStyleable is False
    assert str(value.getStorageKey()) == "value"


def test_store_backed_property_is_described():
    track_width = slider_info().findProperty("trackWidth")

    assert track_width is not None
    assert track_width.type == yup.ComponentProperty.Type.number
    assert track_width.isStoreBacked()
    assert not track_width.isStateBacked()
    assert not track_width.isReadOnly()
    assert track_width.isStyleable is True
    assert track_width.isInherited is True
    assert track_width.layoutEffect == yup.ComponentProperty.LayoutEffect.none
    assert track_width.defaultValue == 4.0
    assert track_width.description == "The thickness of the linear track."

    # A store-backed value is kept under the storage id, the one the widget's Style names.
    assert str(track_width.storageId) == "sliderTrackWidthId"
    assert str(track_width.getStorageKey()) == "sliderTrackWidthId"


def test_read_only_and_state_properties_are_flagged():
    info = slider_info()

    assert info.findProperty("normalized").isReadOnly()

    dragging = info.findProperty("dragging")
    assert dragging.type == yup.ComponentProperty.Type.boolean
    assert dragging.isState is True
    assert dragging.isReadOnly()


def test_enumeration_property_lists_its_values():
    slider_type = slider_info().findProperty("sliderType")

    assert slider_type.type == yup.ComponentProperty.Type.enumeration
    assert slider_type.enumValues.contains("linearHorizontal")
    assert slider_type.enumValues.contains("rotary")
    assert slider_type.defaultValue == "linearHorizontal"


def test_properties_are_inherited_from_the_base_type():
    info = slider_info()

    assert info.hasProperty("opacity")
    assert info.findProperty("opacity").isStateBacked()

    # Layout item properties describe one component only, and update the layout of its parent.
    flex_grow = info.findProperty("flexGrow")
    assert flex_grow.isStoreBacked()
    assert flex_grow.isInherited is False
    assert flex_grow.layoutEffect == yup.ComponentProperty.LayoutEffect.parent

    own = names_of(info.getOwnProperties())
    assert "value" in own
    assert "opacity" not in own

    everything = names_of(info.getAllProperties())
    assert "value" in everything
    assert "opacity" in everything
    assert len(everything) == len(set(everything))


def test_unknown_property_is_not_found():
    info = slider_info()

    assert info.findProperty("doesNotExist") is None
    assert not info.hasProperty("doesNotExist")


def test_property_descriptors_outlive_the_lookup():
    # Descriptors are copies, so they don't depend on the type info wrapper they were read from.
    value = yup.Slider(yup.SliderType.LinearHorizontal).getTypeInfo().findProperty("value")

    assert str(value.name) == "value"


# ==============================================================================
# Signals
# ==============================================================================

def test_signal_is_described():
    info = slider_info()

    assert info.hasSignal("onValueChanged")
    assert names_of(info.getOwnSignals()) == ["onValueChanged"]

    signal = info.findSignal("onValueChanged")
    assert signal is not None
    assert str(signal.name) == "onValueChanged"
    assert signal.description == "Emitted with the new value when the value changes"

    assert len(signal.arguments) == 1
    assert str(signal.arguments[0].name) == "value"
    assert signal.arguments[0].type == yup.ComponentProperty.Type.number
    assert signal.arguments[0].elementType == yup.ComponentProperty.Type.any


def test_unknown_signal_is_not_found():
    info = slider_info()

    assert info.findSignal("onClick") is None
    assert not info.hasSignal("onClick")


def test_signals_are_inherited_from_the_base_type():
    text_button = registry().findType("TextButton")

    assert str(text_button.getBaseTypeInfo().getTypeName()) == "Button"
    assert text_button.getOwnSignals() == []
    assert text_button.hasSignal("onClick")
    assert text_button.findSignal("onClick").description == "Emitted when the button is clicked"


# ==============================================================================
# Registry
# ==============================================================================

def test_registry_lists_the_built_in_types():
    for name in BUILT_IN_TYPES:
        assert registry().findType(name) is not None, name


def test_registry_finds_types_by_name():
    slider = registry().findType("Slider")

    assert slider is not None
    assert str(slider.getTypeName()) == "Slider"
    assert slider is slider_info()

    assert registry().findType("DoesNotExist") is None


def test_registry_returns_the_registered_types_sorted_by_name():
    names = [str(info.getTypeName()) for info in registry().getRegisteredTypes()]

    assert names == sorted(names)

    for name in BUILT_IN_TYPES:
        assert name in names


def test_registry_is_a_single_instance():
    assert yup.ComponentTypeRegistry.getInstance() is yup.ComponentTypeRegistry.getInstance()


# ==============================================================================
# Factory
# ==============================================================================

def test_type_infos_create_instances():
    label_info = registry().findType("Label")
    assert label_info.hasFactory()

    label = label_info.createInstance()
    assert label is not None
    assert label.getTypeName() == "Label"

    slider = registry().findType("Slider").createInstance()
    assert slider is not None
    assert slider.getProperty("sliderType") == "linearHorizontal"


def test_created_instance_is_owned_by_python():
    parent = yup.Component()
    child = registry().findType("Label").createInstance()

    parent.addChildComponent(child)
    assert parent.getNumChildComponents() == 1

    # Destroying the instance takes it out of its parent, as for any component Python created.
    del child
    gc.collect()

    assert parent.getNumChildComponents() == 0


def test_abstract_type_has_no_factory():
    button_info = registry().findType("Button")

    assert not button_info.hasFactory()
    assert button_info.createInstance() is None
