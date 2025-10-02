# Development documentation

## Getting started

The first thing you need to do is the [setup of QtCreator](qtcreator_setup.md).

Then you should read the [coding convention](coding.md).

## Projects and project include files

In the `src/` folder there are several Qt project files use for development:

- `libs.pro` - code and tests of base libraries (cppCORE, cppNGD, cppNGSD, ...)
- `tools.pro` - code and tests of ngs-bits command-line tools
- `tools_gui.pro` - code of GUI tools (GSvar)
- `tools_server.pro` - code and tests of GSvarServer

The folder also contains project include files which are used to share settings between project files:

- `lib.pri` - project include file for libraries
- `test.pri` - project include file for tests
- `app_cli.pri` - project include file for command-line tools
- `app_gui.pri` - project include file for GUI tools
- `base.pri` - project include file cointaining base settings. This file is included by all `.pri`  files above.

## Misc

Other stuff:

* [htslib](build_htslib.md)
* [release checklist](checklist_new_release.md)

[Back to main page](../../README.md)