# TravisCI setup notes

## Files

Files for TravisCI are

* `ngs-bits/.travis.yml`
* `ngs-bits/tools/TravisCI/run_osx.sh` runs the OSX test with default clang (currently not enabled)
* `ngs-bits/tools/TravisCI/run_linux_clang.sh` runs Ubuntu:16.04 clang 
* `ngs-bits/tools/TravisCI/run.sh` runs Ubuntu:16:04 gcc

## Description

The YML file sets up testing using several configurations

 * Ubuntu 16.04 gcc
 * Ubuntu 16.04 clang
 * OSX clang

The SH files contain the commands for:

* dependencies installation
* build
* tests
