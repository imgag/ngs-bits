#GSvar documentation

**NOTE:** This documentation is work in (early) progress. Please contact Marc Sturm if you have suggestions for additional content!

##Sample quality control

The sample information dialog shows the main quality metrics from the NGSD:

* number of reads
* average depth on the target region
* percentage of target region covered at least 20x
* average insert size
* KASP result (sample identity check)
* quality annotation from the NGSD (processed sample)

The tooltip of the numberic metrics conains mean and standard deviation of the metrics for the used processing system.   
Values more than 2 standard deviations from the mean are colored red.  

![alt text](qc1.png)

A low 20x coverage is normally caused by a low read cound and, thus, a low averge depth.
The normal ratio between read count / average depth and 20x coverage can be best seen in an NGSD quality management scatter plot.  
Here an example:

![alt text](qc2.png)


##Variant filtering

TODO

##Misc

### IGV intregration

TODO

### Preferred transcripts

TODO

## FAQ


### IGV does not open
Follow these instructions, if only the black console window of IGV but not the actual application opens.

 - Open the path `C:\Users\[login]\` in the Explorer (replace `[login]` by your Windows login).
 - Delete or rename the `igv` folder (if it cannot be deleted, close all IGV windows).
 - Restart IGV.
 - Accept the genomes cannot be loaded dialog with `ok`.
 - Change the proxy settings as described below in `IGV cannot load genomes`.

### IGV cannot load genomes
IGV needs access to the Broad Institute web server to manage non-local genome files.  
If it cannot access the server, during startup the error message `cannot connect to genome server` is shown and several other errors can occur.

In that case, you have to set the proxy like shown here:
![alt text](igv_proxy.png)

Finally, you have to restart IGV.



