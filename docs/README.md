# Documentation

This documentation is automatically published here: https://antaris-inc.github.io/SatOS-Payload-SDK/.

If you wish to modify the docs, please submit changes to the source files in `src/`.
The framework in use is [Sphinx](https://www.sphinx-doc.org/en/master/).
The [reStructuredText (RST) Primer](https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html) is a useful resource.

## Building the Docs

Simply install sphinx on your machine and use the `sphinx-build` tool to render the source format into HTML.

Additionally, you may need to install 'furo' theme.
```
pip3 install furo
```

This process is documented via `make docs` in the `Makefile` at the root of the repository.

You can see just how this automation works via Github Actions: https://github.com/antaris-inc/SatOS-Payload-SDK/actions
