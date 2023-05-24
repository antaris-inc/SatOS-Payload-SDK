# Configuration file for the Sphinx documentation builder.

# -- Project information

project = 'SatOS Payload SDK'
copyright = '2023 Antaris, Inc'

#release = '0.1'
#version = '0.1.0'

# -- General configuration

extensions = [
    'sphinx.ext.duration',
    'sphinx.ext.doctest',
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.intersphinx',
]

intersphinx_mapping = {
    'python': ('https://docs.python.org/3/', None),
    'sphinx': ('https://www.sphinx-doc.org/en/master/', None),
}
intersphinx_disabled_domains = ['std']

templates_path = ['_templates']

# -- Options for HTML output

html_static_path = ["images"]

html_theme = 'furo'

html_theme_options = {
    "sidebar_hide_name": True,
    "light_logo": "Antaris-Logo-BLK-Trademarked.png",
    "dark_logo": "Antaris-Logo-Green-Trademarked.png",
}

# -- Options for EPUB output
epub_show_urls = 'footnote'
