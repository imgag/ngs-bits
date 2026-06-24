# Running GSvar client application

 We are currently do not provide any official installation images (DMG) or support for Mac OS. If you need a Mac OS version of the GSvar client app, you should build it yourself from the source code in this repository. You may use our test environment configuration as a starting point:
https://github.com/imgag/ngs-bits/blob/master/.github/workflows/mac_machine.yml

However, there is some more work involved in getting a functioning DMG file.

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
* home brew (to install other dependencies)

This documentation assumes that you install dependencies using _Brew_. First of all, you are going to need Qt.

	> brew install qt lzlib pkg-config libxml2 qt-mariadb

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
