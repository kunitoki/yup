# Configuration file for the Sphinx documentation builder.
#
# This builds the YUP documentation set. Sources are authored in MyST Markdown
# so they render both on GitHub and through Sphinx with the Clarity theme.
#
# Full list of options: https://www.sphinx-doc.org/en/master/usage/configuration.html

import datetime

project = "YUP"
copyright = f"{datetime.datetime.now().year}, kunitoki@gmail.com"
author = "kunitoki@gmail.com"

# -- General configuration ---------------------------------------------------

extensions = [
    "myst_parser",
    "sphinx.ext.autosectionlabel",
    "sphinx.ext.intersphinx",
    "sphinxcontrib.mermaid",
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

# Render plain ```mermaid fenced blocks as the sphinxcontrib.mermaid directive.
# This keeps the same source rendering as a diagram both on GitHub (native Mermaid support) and through Sphinx.
myst_fence_as_directive = ["mermaid"]

mermaid_output_format = "png"

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
html_css_files = []
html_theme_options = {
    "light_logo": "_static/logo-light.png",
    "dark_logo": "_static/logo-dark.png",
}
