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

# The colored app icon: unlike the white or black logo it reads on light and dark tabs alike.
html_favicon = "../logos/yup_color_icon.svg"

html_css_files = [
    "shibuya-override.css",
    "rtd-flyout.css",
]

html_theme_options = {
    "accent_color": "blue",
    "color_mode": "auto",
    "github_url": github,
    "nav_socials": ["github"],
    "light_logo": "_static/logo-light.png",
    "dark_logo": "_static/logo-dark.png",
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
            "title": "UI",
            "url": "ui/index",
        },
        {
            "title": "DSP",
            "url": "audio/dsp/index",
        },
        {
            "title": "Audio",
            "url": "audio/index",
        }
    ]
}


# -- Syntax highlighting for the YUP languages ------------------------------
# YUI and YDSP have no Pygments lexers of their own, so small permissive ones
# are defined here. The YDSP keywords follow the compiler's lexer
# (modules/yup_dsp_jit/language/yup_YdspLexer.cpp) and the VS Code grammar.

def setup(app):
    from pygments.lexer import RegexLexer, bygroups, words
    from pygments.token import Comment, Keyword, Name, Number, Operator, Punctuation, String, Text

    class YuiLexer (RegexLexer):
        """A permissive lexer for the YUI declarative UI language."""

        name = "YUI"
        aliases = ["yui"]
        filenames = ["*.yui"]

        tokens = {
            "root": [
                (r"\s+", Text),
                (r"//.*?$", Comment.Single),
                (r"/\*", Comment.Multiline, "comment"),
                (r'"(\\.|[^"\\])*"', String.Double),
                (r"#[0-9a-fA-F]{3,8}\b", Number.Hex),
                (words (("import", "component", "property", "alias", "style", "palette",
                         "tokens", "drawing", "shader", "repeat", "in", "changed", "from", "to"),
                        suffix=r"\b"), Keyword),
                (words (("number", "string", "color", "font", "bool", "point", "rect", "size"),
                        suffix=r"\b"), Keyword.Type),
                (words (("true", "false", "root", "parent", "this"), suffix=r"\b"), Keyword.Constant),
                (r"@[A-Za-z_][\w-]*", Name.Decorator),
                (r"#[A-Za-z_][\w-]*", Name.Label),
                (r"\.[A-Za-z_][\w-]*", Name.Class),
                (r":[A-Za-z_][\w-]*(?=[\s{,>])", Name.Decorator),
                (r"[A-Za-z_]\w*(?=\s*:)", Name.Attribute),
                (r"[A-Za-z_]\w*(?=\s*\()", Name.Function),
                (r"[A-Z]\w*", Name.Class),
                (r"[A-Za-z_]\w*", Name),
                (r"[0-9]+\.[0-9]+(?:[a-z%]+)?", Number.Float),
                (r"[0-9]+(?:[a-z%]+)?", Number.Integer),
                (r"<=>|=>|\.\.|[-+*/%<>=!&|~?]", Operator),
                (r"[{}()\[\];:,.]", Punctuation),
                (r".", Text),
            ],
            "comment": [
                (r"[^*/]+", Comment.Multiline),
                (r"/\*", Comment.Multiline, "#push"),
                (r"\*/", Comment.Multiline, "#pop"),
                (r"[*/]", Comment.Multiline),
            ],
        }

    class YdspLexer (RegexLexer):
        """A lexer for the YDSP JIT-compiled DSP language."""

        name = "YDSP"
        aliases = ["ydsp"]
        filenames = ["*.ydsp"]

        tokens = {
            "root": [
                (r"\s+", Text),
                (r"//.*?$", Comment.Single),
                (r"/\*", Comment.Multiline, "comment"),
                (r"\[\[", Punctuation, "annotation"),
                (r'"(\\.|[^"\\])*"', String.Double),
                (r"(processor|graph|struct|node)(\s+)([A-Za-z_]\w*)",
                 bygroups (Keyword.Declaration, Text, Name.Class)),
                (r"(func)(\s+)([A-Za-z_]\w*)", bygroups (Keyword.Declaration, Text, Name.Function)),
                (words (("if", "else", "for", "in", "match", "return", "emit"), suffix=r"\b"), Keyword),
                (words (("processor", "graph", "node", "connection", "input", "output", "parameter",
                         "stream", "state", "process", "block", "event", "func", "init", "struct",
                         "import", "declare", "as", "let"), suffix=r"\b"), Keyword.Declaration),
                (words (("bool", "float", "float32", "float64", "int", "int32", "int64"), suffix=r"\b"), Keyword.Type),
                (words (("true", "false", "pi", "e", "inf", "nan", "sampleRate", "samplePeriod", "blockSize"),
                        suffix=r"\b"), Keyword.Constant),
                (words (("noteOn", "noteOff", "pitchBend", "pressure", "controlChange", "programChange", "slide"),
                        prefix=r"(?<!\.)\b", suffix=r"\b"), Name.Builtin.Pseudo),
                (words (("abs", "acos", "asin", "atan", "atan2", "ceil", "clamp", "cos", "cosh", "exp", "floor",
                         "fma", "fmod", "lerp", "log", "log10", "max", "mem", "min", "pow", "rint", "select",
                         "sign", "sin", "sinh", "smooth", "sqrt", "size", "tan", "tanh", "trace"),
                        suffix=r"\b"), Name.Builtin),
                (r"0[xX][0-9a-fA-F_]+", Number.Hex),
                (r"0[bB][01_]+", Number.Bin),
                (r"\d[\d_]*\.\d[\d_]*(?:[eE][+-]?\d+)?|\d[\d_]*[eE][+-]?\d+", Number.Float),
                (r"\d[\d_]*", Number.Integer),
                (r"[A-Za-z_]\w*(?=\s*\()", Name.Function),
                (r"[A-Za-z_]\w*", Name),
                (r"<:|:>|->|\.\.|&&|\|\||==|!=|<=|>=|[-+*/%!<>=~@'&|^]", Operator),
                (r"[{}()\[\];:,.]", Punctuation),
                (r".", Text),
            ],
            "annotation": [
                (r"\s+", Text),
                (r"\]\]", Punctuation, "#pop"),
                (r'"(\\.|[^"\\])*"', String.Double),
                (words (("name", "author", "version", "license", "description", "min", "max", "step", "unit",
                         "style", "values", "init", "smoothing", "latency", "mode", "stealing", "priority",
                         "role", "main"), suffix=r"\b"), Name.Attribute),
                (words (("true", "false", "poly", "mono", "oldest", "newest", "none", "last", "low", "high",
                         "voiceActivity"), suffix=r"\b"), Keyword.Constant),
                (r"-?\d[\d_]*(?:\.\d[\d_]*)?(?:[eE][+-]?\d+)?", Number),
                (r"[A-Za-z_]\w*", Name),
                (r"[:,=()\[\]-]", Punctuation),
                (r".", Text),
            ],
            "comment": [
                (r"[^*/]+", Comment.Multiline),
                (r"/\*", Comment.Multiline, "#push"),
                (r"\*/", Comment.Multiline, "#pop"),
                (r"[*/]", Comment.Multiline),
            ],
        }

    app.add_lexer ("yui", YuiLexer)
    app.add_lexer ("ydsp", YdspLexer)
