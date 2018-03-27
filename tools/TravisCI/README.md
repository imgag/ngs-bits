# TravisCI setup notes

## Files

Files for TravisCI are

* `ngs-bits/.travis.yml`
* `ngs-bits/tools/TravisCI/run.sh`

## Description

The YML file sets up testing using a Ubuntu docker image with several compilers, currently:

 * GCC
 * Clang

The SH file contains the commands for:

* dependencies installation
* build
* tests

