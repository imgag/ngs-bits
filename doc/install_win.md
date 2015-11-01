
#Installation instructions for Windows

##Dependencies

First, we need to install some software:

* Download and install [Qt 5.5.0 (32bit, MinGW)](http://www.qt.io/download-open-source/#section-2) to the default location.
* Download and install [Git](https://git-scm.com/download/win).  
  It is needed to extract the repository version during the build process.  
* Download and install [CMake](http://www.cmake.org/cmake/resources/software.html).  
  It is needed to build BamTools. Make sure to add the CMake folder to the path during the installation.
* **Optional:** To create plots in qcML files, install [WinPython](http://winpython.github.io/) and add the python directory to the PATH (it is inside the WinPython directory).

##Building the MySQL driver

The Qt distribution contains an old MySQL driver that does not support binding as it should.  
Thus, we need to build the driver ourselves:

* Download the [MySQL Community Server 5.7.9 ZIP file (32-bit)](http://dev.mysql.com/downloads/mysql/) and extract it to C:\Qt\Qt5.5.0\mysql-5.7.9-win32\.  
* Download the [Qt 5.5.0 sources](http://download.qt.io/archive/qt/5.5/5.5.0/single/) and extract them to C:\Qt\Qt5.5.0\qt-everywhere-opensource-src-5.5.0\.
* Exececute the *mysql.bat* file from the ngs-bits root folder in a Qt shell.
* Copy C:\Qt\Qt5.5.0\mysql-5.7.9-win32\lib\libmysql.dll to C:\Windows\


##Building ngs-bits
Now, we can build the third-party libraries, and then ngs-bits:

* Build the BamTools library by executing the script `bamtools.bat`.  
  [BamTools](http://sourceforge.net/projects/bamtools/) is needed by ngs-bits to access BAM files.
* Finally, build the ngs-bits tools using the QtCreator project file `src\tools.pro`.  
  After a successful build, the tools can be found in the `bin\` folder.

