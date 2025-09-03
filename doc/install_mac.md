# Running GSvar client application

 The Mac version works exclusively on Apple Silicon machines and will not launch on older Intel Macs. At the moment GSvar for Mac is distributed as an individual DMG file (not through the official App Store). It may cause some inconveniences due to the Mac OS privacy and security settings. GSvar needs to be added as an exception, since Mac OS cannot establish or verify where the app comes from. This might change in the future, but for now please follow these steps to launch GSvar:

- Download DMG file
- Double click on the file, you will see the app container (this may take several minutes, since the operating system will be performing some checks)
- Copy GSvar into your `Applications` folder
- Launch GSvar from `Applications` folder or using `Spotlight` (press `cmd` + `space` and type `GSvar`). Your security settings may forbid running apps downloaded outside of the App Store. In this case you will have to go to `Privacy & Security` section of the system settings, scroll down to `Security` and select `Anywhere` or `App Store & Known Developers` in the drop-down list next to the `Allow applications from`. If choosing the `App Store & Known Developers` option does not solve the problem, you will need to disable `Gatekeeper` by executing `sudo spctl --master-disable` in the terminal. After that `Anywhere` option will become available
- During the launch GSvar will ask to perform its automatic configuration, press `Yes`. It will generate `settings.ini` file containing settings tuned to your system

## IGV installation
- Download IGV from the [`official page`](https://igv.org/doc/desktop/#DownloadPage/). Choose the `IGV for MacOS (Apple Chip - Java included)` version
- IGV has to be started, before you launch GSvar. Otherwise GSvar may have problems with the connection to IGV. 

## Integration with IGV

For more details related to IGV, please see the [`IGV installation page`](GSvar\install_igv.md).


# Building ngs-bits from sources MacOS

## Dependencies

ngs-bits depends on the following software to be installed

* _XCode_
* _qmake_ (Qt 6)
* _git_ (to extract the version hash)
* __optional:__ python and matplotlib (for plot generation in QC tools)
* home brew (to install other dependencies)

This documentation assumes that you install dependencies using _Brew_. First of all, you are going to need Qt.

	> brew install qt python@3.10 lzlib pkg-config libxml2 qt-mariadb

If you want to develop under MacOS, you need to install Qt Creator IDE:

	> brew install qt-creator

### Configuring Qt Creator

Having installed Qt Creator, make sure your Qt installation has been correctly found (Preferences->Kits). Usually it is at `/opt/homebrew/opt/qt/bin/qmake` (path may vary, depending on the version number)

### Running the Database Server in a Docker Container

GSvar uses MariaDB as a storage. You don't have to install MySQL/MariaDB server on your machine, you can simply run it in a Docker container instead:

	> docker run -d --name my-own-mysql -p 3306:3306 -e MYSQL_ROOT_PASSWORD=mypass123 mysql:5.7
	> docker run --name my-own-phpmyadmin -d --link my-own-mysql:db -p 8081:80 phpmyadmin/phpmyadmin

PhpMyAdmin can now be accessed from a browser at (use `root` as the username and the password from the above):

	> https://localhost:8081

Just import your database dump and you are ready to go.

## Build

Just execute the following make commands to build the desktop app (the last command is for the tools):

    > make build_3rdparty
	> make build_libs_release
	> make build_tools_release

If you need to build a different version of htslib, please follow [these instructions](build_htslib.md#linux_mac)

## Deployment

Qt comes with a deployment tool for Mac computers. This tool helps finding and copying dependencies of an app. However, it does not work correctly. Some manual work and twicking are necessary afterwards.

For example, to see all the dependencies of GSvar, you may use otool utility:

    > otool -L GSvar.app/Contents/MacOS/GSvar

Fix path values for dependencies first:

    > install_name_tool -change libcppCORE.1.dylib @executable_path/../Frameworks/libcppCORE.1.dylib GSvar.app/Contents/MacOS/GSvar
    > install_name_tool -change libcppXML.1.dylib @executable_path/../Frameworks/libcppXML.1.dylib GSvar.app/Contents/MacOS/GSvar 
    > install_name_tool -change libcppNGS.1.dylib @executable_path/../Frameworks/libcppNGS.1.dylib GSvar.app/Contents/MacOS/GSvar
    > install_name_tool -change libcppGUI.1.dylib @executable_path/../Frameworks/libcppGUI.1.dylib GSvar.app/Contents/MacOS/GSvar
    > install_name_tool -change libcppNGSD.1.dylib @executable_path/../Frameworks/libcppNGSD.1.dylib GSvar.app/Contents/MacOS/GSvar
    > install_name_tool -change libcppVISUAL.1.dylib @executable_path/../Frameworks/libcppVISUAL.1.dylib GSvar.app/Contents/MacOS/GSvar

Check for available signatures (the app needs to be signed):

    > security find-identity -v -p codesigning 

Following these steps to create a Mac bundle:

    > xattr -cr GSvar.app
	> cp -r genomes/ GSvar.app/Contents/MacOS
    > cp GSvar_filters.ini GSvar.app/Contents/MacOS
    > cp GSvar_filters_cnv.ini GSvar.app/Contents/MacOS
    > cp GSvar_filters_sv.ini GSvar.app/Contents/MacOS
    > cp GSvar_special_regions.tsv GSvar.app/Contents/MacOS
    > cp cloud_settings_template.ini GSvar.app/Contents/MacOS
    > codesign --deep --force --verify --verbose --sign  "Developer ID Application: FIRST_NAME LAST_NAME (XXXXXXXX)" GSvar.app
    > macdeployqt GSvar.app -dmg -verbose=2 GSvar.app -libpath=REPOSITORY_LOCATION/ngs-bits/bin
    
It is recommended to have a developer account, which allows distributing your app easier (and even publishing it in the App Store)

## Server update (for cloud instances)

Steps to update an existing cloud instance of GSvar server
	
	> git pull
	> git status
    > git submodule update --recursive --init
	> sudo systemctl stop gsvar.service
    > make build_libs_release build_server_release
    > sudo systemctl start gsvar.service

## Development Environment

GSvar can be build from inside Qt Creator by using its standard mechanisms:

	> tools_gui -> Release -> GSvar
