# Building ngs-bits from sources (on Windows using MSYS2 and MinGW)
The most complicated thing with the Windows build is the support of HTTPS. Configured incorrectly it will make impossible the use of the server API.

## Dependencies

The following depdendemcies have to be installed:

* Download and install [MSYS2](http://www.msys2.org/) at *C:\msys64*.
* Depending on the architecture, you should open either *mingw32.exe* or *mingw64.exe* (located at *C:\msys64*)
* msys2 offers a package manager called *pacman*. Use it to install all the packages from below (*mingw-w64-i686-qt* prefix corresponds to 32-bit system and *mingw-w64-x86_64* to 64-bit system respectively):

    `pacman -S git base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-zlib mingw-w64-x86_64-bzip2 mingw-w64-x86_64-xz mingw-w64-x86_64-curl mingw-w64-x86_64-wget mingw-w64-x86_64-dlfcn mingw-w64-x86_64-qt mingw-w64-x86_64-libmariadbclient mingw-w64-x86_64-python mingw-w64-x86_64-python-numpy mingw-w64-x86_64-python-matplotlib`

## htslib

[This issue](https://github.com/samtools/htslib/issues/907) from the [official github repository](https://github.com/samtools/htslib) provides a lot of information about some potential problems with the Windows build. However, it should be sufficient to follow these steps:
* Download [the latest release of htslib](https://github.com/samtools/htslib/releases/download/1.15/htslib-1.15.tar.bz2) (github page has a section with releases)
* Unpack htslib: *tar -xjf htslib-[VERSION].tar.bz2*
* Configure htslib to enable HTTPS support: `./configure --enable-libcurl --enable-plugins`
* Compile the library: `make`
* Install it into the system (you can copy the binary files to use the libary on a different system): `make install`


## Download

Clone the most recent release of ngs-bits (the source code package of GitHub does not contain required sub-modules):

    git clone --recursive https://github.com/imgag/ngs-bits.git
	cd ngs-bits
	git checkout 2022_04
	git submodule update --recursive --init

## Build
Now you can build ngs-bits as if you are using Linux:
    
    make build_libs_release build_tools_release build_gui_release

## Development
You can also install *Qt creator* and create your own development environment with msys2: `pacman -S mingw-w64-x86_64-qt-creator`

## Build in a Docker container
`docker run -it -v FOLDER_ON_YOUR_MACHINE:/root/output/ win_build:latest`
