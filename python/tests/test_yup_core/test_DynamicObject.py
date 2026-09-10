import yup

#==================================================================================================

def test_new_object_has_no_properties():
    obj = yup.DynamicObject()

    assert not obj.hasProperty("anything")
    assert not obj.hasMethod("anything")

#==================================================================================================

def test_set_and_get_property():
    obj = yup.DynamicObject()

    obj.setProperty("count", 42)

    assert obj.hasProperty("count")
    assert obj.getProperty("count") == 42

#==================================================================================================

def test_property_values_keep_their_type():
    obj = yup.DynamicObject()

    obj.setProperty("intValue", 7)
    obj.setProperty("floatValue", 1.5)
    obj.setProperty("stringValue", "text")

    assert obj.getProperty("intValue") == 7
    assert obj.getProperty("floatValue") == 1.5
    assert obj.getProperty("stringValue") == "text"

#==================================================================================================

def test_missing_property_returns_the_default():
    obj = yup.DynamicObject()

    assert obj.getProperty("missing", "fallback") == "fallback"

#==================================================================================================

def test_setting_a_property_twice_keeps_the_last_value():
    obj = yup.DynamicObject()

    obj.setProperty("value", 1)
    obj.setProperty("value", 2)

    assert obj.getProperty("value") == 2

#==================================================================================================

def test_remove_property():
    obj = yup.DynamicObject()
    obj.setProperty("name", "value")

    obj.removeProperty("name")

    assert not obj.hasProperty("name")

#==================================================================================================

def test_clear_removes_every_property():
    obj = yup.DynamicObject()
    obj.setProperty("a", 1)
    obj.setProperty("b", "two")

    obj.clear()

    assert not obj.hasProperty("a")
    assert not obj.hasProperty("b")
