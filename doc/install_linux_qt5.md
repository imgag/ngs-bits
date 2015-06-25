
#Installing Qt 5.4 on Linux from source

* Download the 64-bit offline installer for Qt5.4 from the [Qt website](https://www.qt.io/download-open-source/#section-2).
* Execute the installer to install Qt.
* Check that the MySQL plugin can find the mysql libary:
 * Check dependencies of the MySQL plugin:  
   `ldd [qt-path]/5.4/gcc_64/plugins/sqldrivers/libqsqlmysql.so`
  * If the libmysqlclient_r.so.16 file cannot be found, the version number is probably wrong. Locate the libary and add a symbolic link:  
   `locate libmysqlclient_r.so`
  * Go the the lib path(s) and add symbolic links with the correct version, e.g.:  
   `cd [lib-path] && ln -s libmysqlclient_r.so.18 libmysqlclient_r.so.16`

