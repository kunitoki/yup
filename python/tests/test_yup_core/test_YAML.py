import yup

#==================================================================================================

def test_parse_mapping_into_python_values():
    parsed = yup.YAML.parse("a: 1\nb: two\n")

    assert parsed["a"] == 1
    assert parsed["b"] == "two"

#==================================================================================================

def test_parse_sequence_into_a_python_list():
    parsed = yup.YAML.parse("- one\n- two\n- three\n")

    assert parsed == ["one", "two", "three"]

#==================================================================================================

def test_round_trip_through_to_string():
    parsed = yup.YAML.parse("a: 1\nb: two\n")
    reparsed = yup.YAML.parse(yup.YAML.toString(parsed))

    assert reparsed["a"] == parsed["a"]
    assert reparsed["b"] == parsed["b"]

#==================================================================================================

def test_to_string_with_format_options():
    parsed = yup.YAML.parse("a: 1\nb: two\n")
    singleLine = yup.YAML.FormatOptions().withSpacing(yup.YAML.Spacing.singleLine)

    text = yup.YAML.toString(parsed, singleLine)

    assert yup.YAML.fromString(text)["a"] == 1

#==================================================================================================

def test_from_string_handles_scalars():
    assert yup.YAML.fromString("42") == 42
    assert yup.YAML.fromString("hello") == "hello"

#==================================================================================================

def test_parse_reports_errors_through_a_result():
    result, parsed = yup.YAML.parseWithResult("a: [1, 2\n")

    assert result.failed()

#==================================================================================================

def test_parse_succeeds_for_valid_input():
    result, parsed = yup.YAML.parseWithResult("a: 1\n")

    assert result.wasOk()
    assert parsed["a"] == 1

#==================================================================================================

def test_format_options_defaults():
    options = yup.YAML.FormatOptions()

    assert options.getSpacing() == yup.YAML.Spacing.multiLine
    assert options.getMaxDecimalPlaces() == 15
    assert options.getIndentLevel() == 0

#==================================================================================================

def test_format_options_are_immutable_and_return_copies():
    options = yup.YAML.FormatOptions()

    changed = options \
        .withSpacing(yup.YAML.Spacing.singleLine) \
        .withMaxDecimalPlaces(3) \
        .withIndentLevel(2)

    # The original is untouched, so the with-er methods return copies.
    assert options.getSpacing() == yup.YAML.Spacing.multiLine
    assert options.getMaxDecimalPlaces() == 15
    assert options.getIndentLevel() == 0

    assert changed.getSpacing() == yup.YAML.Spacing.singleLine
    assert changed.getMaxDecimalPlaces() == 3
    assert changed.getIndentLevel() == 2

#==================================================================================================

def test_escape_string_quotes_extended_characters():
    escaped = yup.YAML.escapeString("say \"hello\"\n")

    assert escaped != ""
    assert "\"" not in escaped or "\\\"" in escaped
