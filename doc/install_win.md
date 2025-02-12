# Building ngs-bits from sources (Windows)

This manual assumes you have already retrieved a local copy of the [github respository](https://github.com/imgag/ngs-bits), as it has been described at [the main page](../README.md)

## Dependencies

First, we need to install Qt and some basic dependencies:

* Download and install [Qt 5.12.12](https://download.qt.io/archive/qt/5.12/5.12.12/) to the default location.  
  Make sure to install `MinGW 7.3.0 64 bit`, `Qt Charts` and `Sources` from the `Qt 5.12.12` components as well. 
* Download and install [Git](https://git-scm.com/download/win).  
  It is needed to extract the repository version during the build process.  
* **Optional:** To create plots in qcML files, install [WinPython](http://winpython.github.io/) and add the python directory to the PATH (it is inside the WinPython directory).

### htslib

[htslib](https://github.com/samtools/htslib) is a third-party library that provides functionality for NGS data formats like BAM or VCF.

We have pre-built `htslib 1.16.1` for Windows and it can be found inside the repository at `ngs-bits\htslib\htslib_win_64.zip`. Just unzip the contents of the ZIP archive into the `ngs-bits\htslib\` folder.

If you want to use a different version, e.g. when testing the latest version of htslib, there is a [manual on how to build htslib](build_htslib.md#windows) available.


### MySQL driver

The Qt distribution does not contains the MySQL driver.  
Thus, we need to install it manually:

* Download the ZIP file of [MySQL Community Server 8.0.31](http://downloads.mysql.com/archives/community/) and extract it to C:\Qt\.  
* Copy C:\Qt\mysql-8.0.31-winx64\lib\libmysql.dll to `ngs-bits\bin`

### MySQL plugin for Qt

The Qt distribution no longer contains a MySQL plugin.

Based on the [official instructions](https://doc.qt.io/qt-5/sql-driver.html#how-to-build-the-qmysql-plugin-on-windows), we have created these updated short instructions:

* Open a Windows CMD window.
* Add the Qt paths to the PATH: *set PATH=C:\Qt\Qt5.12.12\5.12.12\mingw73_64\bin\;C:\Qt\Qt5.12.12\Tools\mingw730_64\bin\;%PATH%*
* Navigate to `C:\Qt\Qt5.12.12\5.12.12\Src\qtbase\src\plugins\sqldrivers\`.
* Execute `qmake -- MYSQL_INCDIR="C:\Qt\mysql-8.0.31-winx64\include" MYSQL_LIBDIR="C:\Qt\mysql-8.0.31-winx64\lib"`
* Execute `mingw32-make sub-mysql`
* Execute `mingw32-make sub-mysql-install_subtargets`

## Build

We can now build ngs-bits:

* Build the ngs-bits tools using the QtCreator project file `src\tools.pro`. Make sure to build in release mode!  
* Then, build GSvar and other GUI tools using the *QtCreator* project file `src\tools_gui.pro`. Make sure to build in release mode!  

*Attention: Make sure to compile the [CRYPT_KEY](../GSvar/encrypt_settings.md) into the GSvar binary when using it in client-server mode. The CRYPT_KEY is used for a handshake between client and server.*

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
		<td>C:\Qt\Qt5.12.12\5.12.12\mingw73_64\bin\</td>
		<td>Qt5Charts.dll, Qt5Core.dll, Qt5Gui.dll, Qt5Network.dll, Qt5PrintSupport.dll, Qt5Sql.dll, Qt5Svg.dll, Qt5Widgets.dll, Qt5Xml.dll, Qt5XmlPatterns.dll, libwinpthread-1.dll, libstdc++-6.dll</td>
	</tr>
	<tr>
		<td>C:\Qt\Qt5.12.12\5.12.12\mingw73_64\plugins\</td>
		<td>platforms, sqldrivers, printsupport, imageformats, bearer, iconengines, styles</td>
	</tr>
	<tr>
		<td>C:\Qt\Qt5.12.12\Tools\mingw730_64\opt\bin\</td>
		<td>ssleay32.dll, libeay32.dll</td>
	</tr>
	<tr>
		<td>C:\Qt\mysql-8.0.31-winx64\lib\</td>
		<td>libmysql.dll</td>
	</tr>
</table>

## Integration with IGV

For all the questions related to IGV, please see the [`IGV installation page`](GSvar\install_igv.md).


