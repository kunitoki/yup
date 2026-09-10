import yup

#==================================================================================================

TRANSLATION_FILE = """language: German
countries: de at

"hello" = "hallo"
"goodbye" = "auf wiedersehen"
"""

#==================================================================================================

def test_language_and_country_codes_are_read_from_the_contents():
    strings = yup.LocalisedStrings(TRANSLATION_FILE, False)

    assert strings.getLanguageName() == "German"
    assert strings.getCountryCodes()[0] == "de"
    assert strings.getCountryCodes()[1] == "at"

#==================================================================================================

def test_known_strings_are_translated():
    strings = yup.LocalisedStrings(TRANSLATION_FILE, False)

    assert strings.translate("hello") == "hallo"
    assert strings.translate("goodbye") == "auf wiedersehen"

#==================================================================================================

def test_unknown_strings_come_back_unchanged():
    strings = yup.LocalisedStrings(TRANSLATION_FILE, False)

    assert strings.translate("not in the table") == "not in the table"

#==================================================================================================

def test_unknown_strings_use_the_fallback_when_one_is_given():
    strings = yup.LocalisedStrings(TRANSLATION_FILE, False)

    assert strings.translate("not in the table", "fallback") == "fallback"

#==================================================================================================

def test_case_sensitivity_follows_the_constructor_flag():
    strings = yup.LocalisedStrings(TRANSLATION_FILE, False)

    # Matching is case sensitive unless ignoreCaseOfKeys was requested.
    assert strings.translate("HELLO") == "HELLO"

#==================================================================================================

def test_case_insensitive_lookup_when_requested():
    strings = yup.LocalisedStrings(TRANSLATION_FILE, True)

    assert strings.translate("HELLO") == "hallo"

#==================================================================================================

def test_mappings_are_exposed():
    strings = yup.LocalisedStrings(TRANSLATION_FILE, False)

    assert isinstance(strings.getMappings(), yup.StringPairArray)

#==================================================================================================

def test_construction_from_a_file(tmp_path):
    path = tmp_path / "translations.txt"
    path.write_text(TRANSLATION_FILE, encoding="utf8")

    strings = yup.LocalisedStrings(yup.File(str(path)), False)

    assert strings.getLanguageName() == "German"
    assert strings.translate("hello") == "hallo"

#==================================================================================================

def test_translating_with_no_current_mappings_returns_the_input():
    # Nothing has called setCurrentMappings(), so this is the pass-through path.
    assert yup.LocalisedStrings.translateWithCurrentMappings("unchanged") == "unchanged"
