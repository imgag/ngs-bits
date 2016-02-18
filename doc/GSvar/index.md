#GSvar documentation

##Sample quality control

The sample information dialog shows the main quality metrics from the NGSD:

* number of reads
* percentage of target region covered at least 20x
* average insert size
* KASP result (sample identity check)
* quality class from NGSD

The tooltip of the numberic metrics conains mean and standard deviation of the metrics for the used processing system.   
Values more than 2 standard deviations from the mean are colored red.  

![alt text](qc1.png)

A low 20x coverage is normally caused by a low read cound and, thus, a low averge depth.
The normal ratio between read count / average depth and 20x coverage can be best seen in an NGSD quality management scatten plot.  
Here an example:

![alt text](qc2.png)


##Variant filtering

TODO

##Misc

### IGV intregration

TODO

### Preferred transcripts

TODO

## IGV

### IGV proxy settings
IGV needs access to the Broad Institute web server to manage non-local genome files.  
If it cannot access the server, during startup the error message `cannot connect to genome server` is shown and several other errors can occur.

In that case, you have to set the proxy like shown here:
![alt text](igv_proxy.png)

