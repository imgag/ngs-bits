# Integrating IGV with GSvar

1. Download `Command line IGV and igvtools for all platforms` from the [official page](https://igv.org/doc/desktop/#DownloadPage/)

2. Unpack the archive

3. Download a JDK, you may try the [official one](jdk.java.net/archive/). Select the version which is required by IGV (newer version may need newer JDKs)

4. Unpack the JDK archive to the folder with IGV (next to the `lib` folder)

5. Make sure that `igv.bat` file contains the correct JDK location (it can be slightly different, depending on the release number)

6. Try starting IGV by executing `igv.bat` file

7. Modify the path to IGV in GSvar config file `GSvar.ini`: adjust `igv_app` property accordingly

8. Now it should be possible to launch IGV from GSvar. If you are behind a authenticating proxy and IGV cannot be launched from GSvar, you have to comment out `jdk.http.auth.tunneling.disabledSchemes=Basic` line in `net.properties` of JDK.