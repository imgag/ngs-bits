
# Building ngs-bits from sources (Windows)

## Dependencies

First, we need to install some software:

* Download and install [Qt 5.9.5](http://download.qt.io/archive/qt/5.9/5.9.5/) to the default location.  
  Make sure to install `MinGW 5.3.0 32 bit` and `Qt Charts` from the `Qt 5.9.5` components as well. 
* Download and install [Git](https://git-scm.com/download/win).  
  It is needed to extract the repository version during the build process.  
* **Optional:** To create plots in qcML files, install [WinPython](http://winpython.github.io/) and add the python directory to the PATH (it is inside the WinPython directory).

### MySQL driver

The Qt distribution contains an old MySQL driver that does not support binding as it should.  
Thus, we need to use a different driver:

* Download the [MySQL Community Server 5.7.9 ZIP file (32-bit)](http://downloads.mysql.com/archives/community/) and extract it to C:\Qt\Qt5.9.5\mysql-5.7.9-win32\.  
* Copy C:\Qt\Qt5.9.5\mysql-5.7.9-win32\lib\libmysql.dll to C:\Windows\

## Download

Open a *Git CMD* and clone the most recent release (the source code package of GitHub does not contain required sub-modules):

    git clone --recursive https://github.com/imgag/ngs-bits.git
	cd ngs-bits
	git checkout 2021_03
	git submodule update --recursive --init

## Build

We can now build ngs-bits:

* Unzip the [htslib](https://github.com/samtools/htslib) headers and DLL from `ngs-bits\htslib\htslib_win.zip`.
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
		<td>hts-2.dll, hts.dll.a</td>
	</tr>
	<tr>
		<td>C:\Qt\Qt5.9.5\5.9.5\mingw53_32\bin\</td>
		<td>Qt5Charts.dll, Qt5Core.dll, Qt5Gui.dll, Qt5Network.dll, Qt5PrintSupport.dll, Qt5Sql.dll, Qt5Widgets.dll, Qt5Xml.dll, Qt5XmlPatterns.dll, libgcc_s_dw2-1.dll, libwinpthread-1.dll, libstdc++-6.dll</td>
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
		<td>C:\Qt\Qt5.9.5\mysql-5.7.9-win32\lib\</td>
		<td>libmysql.dll</td>
	</tr>
</table>
