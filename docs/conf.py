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
# This keeps the same source rendering as a diagram both on GitHub (native
# Mermaid support) and through Sphinx.
myst_fence_as_directive = ["mermaid"]

# -- Mermaid configuration ---------------------------------------------------
#
# Pin a recent Mermaid and configure the client-side renderer. The two settings
# that matter most for legible, correctly-sized diagrams:
#   * flowchart.htmlLabels = false forces SVG-native <text> labels. HTML
#     (foreignObject) labels are measured with the browser's default font but
#     rendered with the theme font, which makes text overflow / misalign with
#     node boxes. SVG text is measured and rendered with the same font.
#   * fontFamily + themeVariables.fontSize set an explicit font so Mermaid's
#     text measurement matches what is drawn (no tiny or clipped labels).
mermaid_version = "11.6.0"

mermaid_init_js = """mermaid.initialize({
    startOnLoad: true,
    securityLevel: 'loose',
    fontFamily: 'trebuchet ms, Segoe UI, Helvetica, Arial, sans-serif',
    flowchart: { htmlLabels: false, useMaxWidth: true, curve: 'basis' },
    themeVariables: { fontSize: '16px' }
});"""

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
