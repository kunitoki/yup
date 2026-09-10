import yup


# ==============================================================================
# ApplicationTheme
# ==============================================================================

def test_global_theme_is_available_while_the_app_runs(juce_app):
    theme = yup.ApplicationTheme.getGlobalTheme()
    assert isinstance(theme, yup.ApplicationTheme)


def test_default_fonts_are_exposed(juce_app):
    theme = yup.ApplicationTheme.getGlobalTheme()

    # The icon and monospace fonts depend on the YUP_EMBED_DEFAULT_THEME_* config, so only
    # their type is checked here; the text font always falls back to a system font.
    for font in (theme.getDefaultFont(), theme.getDefaultIconFont(), theme.getDefaultMonospaceFont()):
        assert isinstance(font, yup.Font)

    assert not theme.getDefaultFont().isEmpty()


def test_default_font_is_a_copy_that_withHeight_does_not_mutate(juce_app):
    theme = yup.ApplicationTheme.getGlobalTheme()

    original = theme.getDefaultFont().getHeight()
    resized = theme.getDefaultFont().withHeight(original + 10.0)

    assert resized.getHeight() == original + 10.0
    assert theme.getDefaultFont().getHeight() == original
