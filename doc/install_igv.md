# Integration with IGV

## Deploying a new version

* Download IGV from the [`official page`](https://igv.org/doc/desktop/#DownloadPage/) (select `Command line IGV and igvtools for all platforms` variant)
* Download a JDK that is reccomended for this particular version of IGV: IGV developers include JDK from [`Adoptium`](https://adoptium.net/de/marketplace/)
* Place the unpacked JDK to the folder with IGV and edit `igv.bat` (for Windows). Change the last line to `start %JAVA_CMD% -showversion --module-path=%BatchPath%\lib -Xmx8g -Dproduction=true @%BatchPath%\igv.args -Djava.net.preferIPv4Stack=true -Dsun.java2d.noddraw=true --module=org.igv/org.broad.igv.ui.Main  %*`
* `conf/net.properties` file inside the JDK folder has to be adjusted: comment out the line `jdk.http.auth.tunneling.disabledSchemes=Basic`. This will enable the correct proxy authentication method.

## Configuring GSvar to use IGV
To link IGV to the GSvar app, edit `igv_app` parameter in the config file. It should point to the script that starts IGV, which may be different depending on your operating system. For Windows it is `igv.bat`, for Mac and Linux a Bash script `igv.sh` is used.

## Building a custom genome for IGV

To generate a custom genome for IGV with all Ensembl transcripts use the megSAP script [`create_igv_genome.php`](https://github.com/imgag/megSAP/blob/master/src/Auxilary/create_igv_genome.php).  
The created JSON genome file must only be set as `igv_genome` in the `GSvar.ini` and then will always be loaded when you use IGV through GSvar.