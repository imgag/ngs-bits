
# Building ngs-bits from sources (Windows)

## Dependencies

First, we need to install some software:

* Download and install [Qt 5.5.0 (32bit, MinGW)](http://download.qt.io/archive/qt/5.5/5.5.0/) to the default location.
* Download and install [Git](https://git-scm.com/download/win).  
  It is needed to extract the repository version during the build process.  
* **Optional:** To create plots in qcML files, install [WinPython](http://winpython.github.io/) and add the python directory to the PATH (it is inside the WinPython directory).

### MySQL driver

The Qt distribution contains an old MySQL driver that does not support binding as it should.  
Thus, we need to use a different driver:

* Download the [MySQL Community Server 5.7.9 ZIP file (32-bit)](http://downloads.mysql.com/archives/community/) and extract it to C:\Qt\Qt5.5.0\mysql-5.7.9-win32\.  
* Copy C:\Qt\Qt5.5.0\mysql-5.7.9-win32\lib\libmysql.dll to C:\Windows\

## Download

Open a *Git CMD* and clone the most recent release (the source code package of GitHub does not contains required sub-modules):

    git clone --recursive https://github.com/imgag/ngs-bits.git
	cd ngs-bits
	git checkout 2018_04
	git submodule update --recursive --init

## Build
Now, we can build ngs-bits:

* Unzip the [htslib](https://github.com/samtools/htslib) headers and DLL from `ngs-bits\htslib\build_win.zip`.
* Build the ngs-bits tools using the QtCreator project file `src\tools.pro`.  
* Then, build GSvar and other GUI tools using the *QtCreator* project file `src\tools_gui.pro`.  
* Finally, you have to copy some DLLs to the `bin`folder:

	<table>
		<tr>
			<td>from path</td>
			<td>copy DLLs</td>
		</tr>
		<tr>
			<td>ngs-bits\htslib\lib\</td>
			<td>hts-2.dll, hts.dll.a</td>
		</tr>
		<tr>
			<td>C:\Qt\Qt5.5.0\5.5\mingw492_32\bin\</td>
			<td>Qt5Core.dll, Qt5XmlPatterns.dll, Qt5PrintSupport.dll, Qt5Network.dll, Qt5Sql.dll, Qt5Xml.dll, Qt5Gui.dll, Qt5Widgets.dll, libgcc_s_dw2-1.dll, libwinpthread-1.dll, libstdc++-6.dll</td>
		</tr>
		<tr>
			<td>C:\Qt\Qt5.5.0\mysql-5.7.9-win32\lib\</td>
			<td>libmysql.dll</td>
		</tr>
	</table>

## Executing

Now the executables and all required libraries can be found in the `bin` folder.

