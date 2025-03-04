# Building ngs-bits from sources MacOS

## Dependencies

ngs-bits depends on the following software to be installed

* _XCode_
* _qmake_ (Qt 5.12 or higher, including xmlpatterns and mysql package)
* _git_ (to extract the version hash)
* __optional:__ python and matplotlib (for plot generation in QC tools)
* __also optional:__ home brew (makes easier to install other depdendencies)

This documentation assumes that you install dependencies using _Brew_. First of all, you are going to need Qt. Since the default version in _Brew_ is currently 6, you need to force it to use version 5 (ngs-bits cannot be compiled with Qt 6):

	> brew install qt@5

If you want to develop under MacOS, you need to install Qt Creator IDE:

	> brew install qt-creator

### Configuring Qt Creator

Having installed Qt Creator, Qt binary location should be specified. Usually it is at `/usr/local/Cellar/qt@5/5.15.2_1/bin/qmake` (path may vary, depending on the version number)

### MySQL Plugin

To enable the database related functionality, a MySQL plugin has to be built. For the most other platforms it is avaliable as a binary package, but not for MacOS. On MacOS you need to
get Qt source code and to compile the plugin by your self:

	> git clone git://code.qt.io/qt/qt5.git
	> git checkout 5.15
	> git submodule update --recursive --init

The last step may take a lot of time, since some module are huge. It is recommended to exclude WebEngine modules. Now you need to go the _qtbase_ module and find the plugin:

	> cd qtbase/src/plugins/sqldrivers

Compile and install the plugin:

	> qmake
	> make sub-mysql
	> make install

### Running the Database Server in a Docker Container

You don't have to install MySQL server on your machine, you can run it in a Docker container instead:

	> docker run -d --name my-own-mysql -p 3306:3306 -e MYSQL_ROOT_PASSWORD=mypass123 mysql:5.7
	> docker run --name my-own-phpmyadmin -d --link my-own-mysql:db -p 8081:80 phpmyadmin/phpmyadmin

PhpMyAdmin can now be accessed from a browser at (use `root` as the username and the password from the above):

	> https://localhost:8081

Just import your database dump and you are ready to go.

## Build

Just execute the following make commands:

    > brew install lzlib pkg-config libxml2
    > make build_3rdparty
	> make build_libs_release
	> make build_tools_release

If you need to build a different version of htslib, please follow [these instructions](build_htslib.md#linux_mac)

## Deployment

For yet unknown reasons, GSvar could not detect *.dylib files located at the same folder on testing machines. It searches for the libraries at /usr/local/lib instead.

Currently used temporary fix looks like that:
 
	> cp libcppXML.1.0.dylib /usr/local/lib/libcppXML.1.dylib
	> cp libcppGUI.1.0.dylib /usr/local/lib/libcppGUI.1.dylib
	> cp libcppNGS.1.0.dylib /usr/local/lib/libcppNGS.1.dylib
	> cp libcppNGSD.1.0.dylib /usr/local/lib/libcppNGSD.1.dylib
	> cp libcppVISUAL.1.0.dylib /usr/local/lib/libcppVISUAL.1.dylib
	> cp libcppCORE.1.0.dylib /usr/local/lib/libcppCORE.1.dylib

All settings should be saved at `/Users/megalex/github/ngs-bits/bin/GSvar.app/Contents/MacOS`

## Development Environment

GSvar can be build from inside Qt Creator by using its standard mechanisms:

	> tools_gui -> Release -> GSvar

_Note:_ The above mentioned processes have been tested on Intel-based Macs. It remains unknown if GSvar can be compiled natively for M1 CPUs (Qt 5 does not officially/completly support M1 chips yet). However, they claim it works through the Rosetta translation layer, native arm64 support seems to be under development. Qt 6 should officially support Apple Silicon.

## Integration with IGV

For all the questions related to IGV, please see the [`IGV installation page`](GSvar\install_igv.md).
