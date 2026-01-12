# Installing new versions of IGV

IGV is regularly updated (usually every couple of months). If you want to install the latest version of IGV, we recommend testing it first, (especially if you use a proxy server to access your data). Pay attention to the proxy configuration and to the BAM/CRAM tracks, since these are the most problematic things in IGV (at least while using it with `GSvar`).

## Deployment

1. Download `Command line IGV and igvtools for all platforms` from the [official page](https://igv.org/doc/desktop/#DownloadPage/)

2. Unpack the archive

3. Download Open JDK, [Oracle Open JDK](jdk.java.net/archive/) is one of the most popular options. Select the version number which is required by IGV (newer version may need newer JDKs). There are many other Open JDK flavours: [`Adoptium Open JDK`](https://adoptium.net/de/releases/), [Microsoft Open JDK](https://learn.microsoft.com/en-us/java/openjdk/download), [Amazon Open JDK](https://downloads.corretto.aws/#/overview), etc.

4. Unpack the JDK archive to the folder with IGV (next to the `lib` folder)

5. Make sure that `igv.bat` file contains the correct JDK location (it can be slightly different, depending on the release number)

6. Try starting IGV by executing `igv.bat` file (for Mac and Linux the Bash script called `igv.sh` is used)

7. Modify the path to IGV in GSvar config file `GSvar.ini`: adjust `igv_app` property accordingly

8. Now it should be possible to launch IGV from GSvar. If you are behind a authenticating proxy and IGV cannot be launched from GSvar, you have to comment out `jdk.http.auth.tunneling.disabledSchemes=Basic` line in `net.properties` of JDK.

## Building a custom genome for IGV

To generate a custom genome for IGV with all Ensembl transcripts use the megSAP script [`create_igv_genome.php`](https://github.com/imgag/megSAP/blob/master/src/Auxilary/create_igv_genome.php).  
The created JSON genome file must only be set as `igv_genome` in the `GSvar.ini` and then will always be loaded when you use IGV through GSvar.
