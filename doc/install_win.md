# Building ngs-bits from sources (Windows)

This manual assumes you have already retrieved a local copy of the [github respository](https://github.com/imgag/ngs-bits), as it has been described at [the main page](../README.md)

## Dependencies

First, we need to install some software. If a standard Qt is used (not from MSYS2):

* Download and install [Qt 5.9.5](http://download.qt.io/archive/qt/5.9/5.9.5/) to the default location.  
  Make sure to install `MinGW 5.3.0 32 bit` and `Qt Charts` from the `Qt 5.9.5` components as well. 
* Download and install [Git](https://git-scm.com/download/win).  
  It is needed to extract the repository version during the build process.  
* **Optional:** To create plots in qcML files, install [WinPython](http://winpython.github.io/) and add the python directory to the PATH (it is inside the WinPython directory).


Alternatively, the dependencies can be installed by means of [MSYS2](http://www.msys2.org/):
* First, download and install [MSYS2](http://www.msys2.org/) itself at *C:\msys64*.
* Depending on the architecture, you should open either *mingw32.exe* or *mingw64.exe* (located at *C:\msys64*). Keep in mind that the support of 32-bit systems is deprecated and soon will be dropped completely.
* MSYS2 offers a package manager called *pacman*. Use it to install all the packages from below (*mingw-w64-i686-qt* prefix corresponds to 32-bit system and *mingw-w64-x86_64* to 64-bit system respectively):

    `pacman -S git base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-zlib mingw-w64-x86_64-bzip2 mingw-w64-x86_64-xz mingw-w64-x86_64-curl mingw-w64-x86_64-wget mingw-w64-x86_64-dlfcn mingw-w64-x86_64-qt5 mingw-w64-x86_64-libmariadbclient mingw-w64-x86_64-python mingw-w64-x86_64-python-numpy mingw-w64-x86_64-python-matplotlib`


### htslib

Htslib is a third-party libarary that provides some important functionality. The Windows binaries can be found inside the repository at `ngs-bits\htslib\htslib_win.zip` (32-bit version) and at `ngs-bits\htslib\htslib_win_64.zip` (64-bit version).
* Unzip the [htslib](https://github.com/samtools/htslib) headers and DLL from the corresponding archive file.

Sometimes one may need to build binaries of the newer version. There is a separate [manual on how to build htslib](https://github.com/imgag/ngs-bits/tree/master/tools/htslib) available. However, it should be sufficient to follow these steps, if you have MSYS2 already installed:
* Download [the latest release of htslib](http://www.htslib.org/download/)
* Unpack htslib: *tar -xjf htslib-[VERSION].tar.bz2*
* Configure htslib to enable HTTP/HTTPS support: `./configure --enable-libcurl`
* Compile the library: `make`
* Install it into the system (you can copy the binary files afterwards to use the libary on a different system): `make install`


### MySQL driver

The Qt distribution contains an old MySQL driver that does not support binding as it should.  
Thus, we need to use a different driver:

* Download the [MySQL Community Server ZIP file](http://downloads.mysql.com/archives/community/) ((32-bit or 64-bit version, based on your architecture)) and extract it to C:\Qt\Qt5.9.5\mysql-*\.  
* Copy C:\Qt\Qt5.9.5\mysql-*\lib\libmysql.dll to C:\Windows\

### Database plugin
Qt distributions do not come with MySQL plugins by default. You will have to build one for your version of MySQL. Qt developers provide very good instructions on how to do that [here](https://doc.qt.io/qt-5/sql-driver.html#how-to-build-the-qmysql-plugin-on-windows). If you are using MSYS2, you will have to add the paths for qmake and C compilers to the `PATH` environment variable.

## Build

We can now build ngs-bits:

* Build the ngs-bits tools using the QtCreator project file `src\tools.pro`. Make sure to build in release mode!  
* Then, build GSvar and other GUI tools using the *QtCreator* project file `src\tools_gui.pro`. Make sure to build in release mode!  


Now the executables can be found in the `bin` folder and can be executed from *QtCreator*.  
To use GSvar, it needs to be [configured](GSvar/configuration.md) first.

## Making the ngs-bits tools portable

To make the tools executable outside *QtCreator* and portable, you have to copy some files/folders into the `bin` folder:

<table>
	<tr>
		<td><b>from path</b></td>
		<td><b>copy</b></td>
	</tr>
	<tr>
		<td>ngs-bits\htslib\lib\</td>
		<td>
		*.*
		</td>
	</tr>
	<tr>
		<td>C:\Qt\Qt5.9.5\5.9.5\mingw53_32\bin\</td>
		<td>Qt5Charts.dll, Qt5Core.dll, Qt5Gui.dll, Qt5Network.dll, Qt5PrintSupport.dll, Qt5Sql.dll, Qt5Widgets.dll, Qt5Xml.dll, Qt5XmlPatterns.dll, libwinpthread-1.dll, libstdc++-6.dll</td>
	</tr>
	<tr>
		<td>C:\Qt\Qt5.9.5\5.9.5\mingw53_32\plugins\</td>
		<td>platforms, sqldrivers, printsupport</td>
	</tr>
	<tr>
		<td>C:\Qt\Qt5.9.5\Tools\mingw530_32\opt\bin\</td>
		<td>ssleay32.dll, libeay32.dll</td>
	</tr>
	<tr>
		<td>C:\Qt\Qt5.9.5\mysql-5.7.39-win32\lib\</td>
		<td>libmysql.dll</td>
	</tr>
</table>
