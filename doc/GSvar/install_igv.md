# Installing new versions of IGV

IGV is regularly updated. If you want to install the latest version of IGV, we recommend testing it first, (especially if you use a proxy server to access your data). Pay attention to the proxy configuration and to the BAM/CRAM tracks, since these are the most problematic things in IGV (while using it with `GSvar`).

## Deployment

* Download IGV 2.19.7 (the latest version available at the moment) from the [`official page`](https://igv.org/doc/desktop/#DownloadPage/) (select `Command line IGV and igvtools for all platforms` variant)
* Download a JDK: IGV developers include JDK from [`Adoptium`](https://adoptium.net/de/releases/), but you may use a different one, just make sure the version number meets the requirement for a given IGV release (check out [Microsoft Open JDK](https://learn.microsoft.com/en-us/java/openjdk/download), [Amazon Open JDK](https://downloads.corretto.aws/#/overview), or some other)
* Place the unpacked JDK to the folder with IGV and edit `igv.bat` (for Windows). Change the last line to `start %JAVA_CMD% -showversion --module-path=%BatchPath%\lib -Xmx8g -Dproduction=true @%BatchPath%\igv.args -Djava.net.preferIPv4Stack=true -Dsun.java2d.noddraw=true --module=org.igv/org.broad.igv.ui.Main  %*`
* The file `conf/net.properties` inside the JDK folder has to be adjusted: comment out the line `jdk.http.auth.tunneling.disabledSchemes=Basic`. This will enable the correct proxy authentication method.

## Configuring GSvar to use IGV
To link IGV to the GSvar app, edit `igv_app` parameter in the config file. It should point to the script that starts IGV, which may be different depending on your operating system. For Windows it is `igv.bat`, for Mac and Linux a Bash script `igv.sh` is used.

## Building a custom genome for IGV

To generate a custom genome for IGV with all Ensembl transcripts use the megSAP script [`create_igv_genome.php`](https://github.com/imgag/megSAP/blob/master/src/Auxilary/create_igv_genome.php).  
The created JSON genome file must only be set as `igv_genome` in the `GSvar.ini` and then will always be loaded when you use IGV through GSvar.
