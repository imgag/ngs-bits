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

--
[back to main page](index.md)

