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

    > make build_3rdparty
	> make build_libs_release
	> make build_tools_release

If you need to build a different version of htslib, please follow [these instructions](build_htslib.md#linux_mac)

## Deployment

Qt comes with a deployment tool for Mac computers. This tool helps finding and copying dependencies of an app. However, it does not work correctly. Some manual work and twicking are necessary afterwards (possibly works better in Qt 6, which runs natively on Apple Silicon nachines). Following these steps to create a Mac bundle: 
   
    > /opt/homebrew/Cellar/qt@5/5.15.16/bin/macdeployqt GSvar.app -dmg -verbose=2 GSvar.app -libpath=/Users/megalex/github/ngs-bits/bin
    > xattr -cr GSvar.app
    > install_name_tool -change /opt/homebrew/opt/mysql/lib/libmysqlclient.24.dylib @executable_path/../Frameworks/libmysqlclient.24.dylib GSvar.app/Contents/PlugIns/sqldrivers/libqsqlmysql.dylib
	> cp -r genomes/ GSvar.app/Contents/MacOS
    > cp GSvar_filters.ini GSvar.app/Contents/MacOS
    > cp GSvar_filters_cnv.ini GSvar.app/Contents/MacOS
    > cp GSvar_filters_sv.ini GSvar.app/Contents/MacOS
    > cp GSvar_special_regions.tsv GSvar.app/Contents/MacOS
    > cp cloud_settings_template.ini GSvar.app/Contents/MacOS
    > codesign --deep --force --options runtime --sign "Apple Development: FIRST_NAME LAST_NAME (XXXXXXXX)" GSvar.app
The last step sings the app with a digital signature. It is recommended to have a devepers account, which allows distributing your app easier (and even publishing it in the App Store)

## Server update (for cloud instances)

Steps to update an existing cloud instance of GSvar server
	
	> git pull
	> git status
    > git submodule update --recursive --init
	> sudo systemctl stop gsvar.service
    > make build_libs_release build_server_release
    > sudo systemctl start gsvar.service


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

## Running GSvar client app

At the moment GSvar is distributed as an individual DMG file (not through the official App Store). It may cause some inconveniencies due to the Mac OS privacy and security settings. GSvar needs to be added as an exception, since Mac OS cannot establish or verify where the app comes from. Follow these steps to launch GSvar:

- Download DMG file
- Double click on the file, you will see the app container (this may take several minutes, since the operating system will be performing some checks)
- Copy-paste or drag-and-drop GSvar into your `Applications` folder
- Launch GSvar from `Applications` folder or using `Spotlight`. Your security settings may forbid running apps downloaded outside of the App Store. In this case you will have to go to `Privacy & Security` section of the system settings, scroll down to `Security` and select `Anywhere` or `App Store & Known Developers` in the drop-down list next to the `Allow applications from`. If choosing the `App Store & Known Developers` option does not solve the problem, you will need to disable `Gatekeeper` by executing `sudo spctl --master-disable` in the terminal. After that `Anywhere` option will become available
- During the launch GSvar will ask to perform its automatic configuration, press `Yes`. It will generate `settings.ini` file containing settings tuned to your system

## IGV installation
- Download IGV from the [`official page`](https://igv.org/doc/desktop/#DownloadPage/). Choose the `Command line IGV and igvtools for all platforms` version
- If you do not have Java 21 or greater installed, run `brew install openjdk`, and follow the instructions (pay attention to the $PATH variable). If Java is installed correctly, you should be able to run `java -version` in your terminal
- IGV has to be started, before you launch GSvar. In the terminal run `[FULL_PATH]/igv.sh --port 61152` script from the IGV folder. IGV will be listening to commands from GSvar. If there are problems with IGV, you should be able to see them in the terminal. 

## Integration with IGV

For more details related to IGV, please see the [`IGV installation page`](GSvar\install_igv.md).
