# Configuration file for the Sphinx documentation builder.
#
# This builds the YUP documentation set. Sources are authored in MyST Markdown
# so they render both on GitHub and through Sphinx with the Shibuya theme.
#
# Full list of options: https://www.sphinx-doc.org/en/master/usage/configuration.html

import datetime

project = "YUP!"
copyright = f"{datetime.datetime.now().year}, kunitoki@gmail.com"
author = "kunitoki@gmail.com"
github = "https://github.com/kunitoki/yup"

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
mermaid_version = "11.6.0"
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

html_theme = "shibuya"
html_title = "YUP Documentation"
html_static_path = ["_static"]

html_css_files = [
    "readthedocs-flyout.css",
]

html_theme_options = {
    "accent_color": "indigo",
    "color_mode": "auto",
    "github_url": github,
    "nav_socials": ["github"],
    "light_logo": "logo-light.png",
    "dark_logo": "logo-dark.png",
    # "announcement": "The content of the announcement",
    "nav_links": [
        {
            "title": "Getting Started",
            "url": "getting-started/index",
        },
        {
            "title": "Build System",
            "url": "build-system/index",
        },
        {
            "title": "Core",
            "url": "core/index",
        },
        {
            "title": "Graphics",
            "url": "graphics/index",
        },
        {
            "title": "Imaging",
            "url": "imaging/index",
        },
        {
            "title": "Audio",
            "url": "audio/index",
        }
    ]
}
