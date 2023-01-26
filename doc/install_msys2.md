# Building ngs-bits from sources (on Windows using MSYS2 and MinGW)
The most complicated thing with the Windows build is the support of HTTPS. Configured incorrectly it will make impossible the use of the server API.

## Virutal Machine Setup (Optional)
Microsoft provides for free a virtual machine images of [Windows 11 development envoronment](https://developer.microsoft.com/en-us/windows/downloads/virtual-machines/) for different virtualization systems. A free [VirtualBox](https://www.virtualbox.org/) solution seems to be great option and works extemely well for the purose of building a Windows executable of GSvar. The VM image includes a preconfigured system which can be used for a limited period of time. Afterwards it is possible to download a new VM image.

## Dependencies

The following depdendemcies have to be installed:

* Download and install [MSYS2](http://www.msys2.org/) at *C:\msys64*.
* Depending on the architecture, you should open either *mingw32.exe* or *mingw64.exe* (located at *C:\msys64*)
* msys2 offers a package manager called *pacman*. Use it to install all the packages from below (*mingw-w64-i686-qt* prefix corresponds to 32-bit system and *mingw-w64-x86_64* to 64-bit system respectively):

    `pacman -S git base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-zlib mingw-w64-x86_64-bzip2 mingw-w64-x86_64-xz mingw-w64-x86_64-curl mingw-w64-x86_64-wget mingw-w64-x86_64-dlfcn mingw-w64-x86_64-qt5 mingw-w64-x86_64-libmariadbclient mingw-w64-x86_64-python mingw-w64-x86_64-python-numpy mingw-w64-x86_64-python-matplotlib`

## htslib

There is a separate [manual on how to build htslib](https://github.com/imgag/ngs-bits/tree/master/tools/htslib) available. However, it should be sufficient to follow these steps, if you have MSYS2 already installed:
* Download [the latest release of htslib](http://www.htslib.org/download/)
* Unpack htslib: *tar -xjf htslib-[VERSION].tar.bz2*
* Configure htslib to enable HTTP/HTTPS support: `./configure --enable-libcurl`
* Compile the library: `make`
* Install it into the system (you can copy the binary files afterwards to use the libary on a different system): `make install`

## Download

Clone the most recent release of ngs-bits (the source code package of GitHub does not contain required sub-modules):

    git clone --recursive https://github.com/imgag/ngs-bits.git
	cd ngs-bits
	git checkout 2022_04
	git submodule update --recursive --init

## MySQL Plugin for Qt

Qt distributions do not come with MySQL plugins by default. You will have to build one for you version of MySQL. Qt developers provide very good instructions on how to do that [here](https://doc.qt.io/qt-5/sql-driver.html#how-to-build-the-qmysql-plugin-on-windows). If you are using MSYS2, you will have to add the paths for qmake and C compilers to the `PATH` environment variable.

## Build
Now you can build ngs-bits as if you are using Linux:
    
    make build_libs_release build_tools_release build_gui_release

## Development
You can also install *Qt creator* and create your own development environment with msys2: `pacman -S mingw-w64-x86_64-qt-creator`. However, in this case the binaries you will genereate may not be compartible with regular Qt DLLs. Always make sure you are not mixing DLLs from different versions of Qt or MINGW.
