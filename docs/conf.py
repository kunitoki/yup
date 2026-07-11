# Configuration file for the Sphinx documentation builder.
#
# This builds the YUP documentation set. Sources are authored in MyST Markdown
# so they render both on GitHub and through Sphinx with the Clarity theme.
#
# Full list of options: https://www.sphinx-doc.org/en/master/usage/configuration.html

project = "YUP"
copyright = "2026, kunitoki@gmail.com"
author = "kunitoki@gmail.com"

# -- General configuration ---------------------------------------------------

extensions = [
    "myst_parser",
    "sphinx.ext.autosectionlabel",
    "sphinx.ext.intersphinx",
]

myst_enable_extensions = [
    "colon_fence",     # ::: fenced directives, GitHub-friendly
    "deflist",
    "fieldlist",
    "linkify",
    "substitution",
    "tasklist",
]

# Auto-generate anchors for headings so cross-file links stay stable.
myst_heading_anchors = 3

autosectionlabel_prefix_document = True

source_suffix = {
    ".md": "markdown",
}

# The root document of the documentation tree.
root_doc = "index"

exclude_patterns = [
    "_build",
    "Thumbs.db",
    ".DS_Store",
    "superpowers",
    "demos",
    "requirements.txt",
]

# -- Options for HTML output -------------------------------------------------

html_theme = "sphinx_clarity_theme"
html_title = "YUP Documentation"
html_static_path = ["_static"]

html_theme_options = {
    "light_logo": "_static/logo-light.png",
    "dark_logo": "_static/logo-dark.png",
}
