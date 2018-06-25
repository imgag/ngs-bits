## Sample quality control

### Assessing quality by NGS QC metrics

The sample details dock widget shows the main quality metrics from the NGSD:

* number of reads
* average insert size
* average depth on the target region
* percentage of target region covered at least 20x
* number of variants
* percentage of known variants
* average deviation from expected allele frequency (0.0, 0.5 or 1.0)
* KASP result (sample identity check)
* quality annotation from the NGSD (processed sample)

The tooltip of the numberic metrics conains mean and standard deviation of the metrics for the used processing system.   
Values more than 2 standard deviations from the mean are colored red.  

![alt text](qc1.png)

A low 20x coverage is normally caused by a low read cound and, thus, a low averge depth.
The normal ratio between read count / average depth and 20x coverage can be best seen in an NGSD quality management scatter plot.  
Here an example:

![alt text](qc2.png)

### Checking sample gender

The sample gender can be easily checked from GSvar:

![alt text](qc_sample_gender.png)

For exomes and panels that contain the SRY gene, please use the method based on the SRY gene.  
If you are not sure if your panel contains the SRY gene, you can check the target region using IGV.  

### Checking sample similarity

If you have related samples and want to check how similar they are, you can do this from GSvar as well:

![alt text](qc_sample_correlation1.png)

The method based on the GSvar file is faster and should be used for most cases.  
SampleSimilarity calculates several metrics that measure how similar two samples are:

 * overlap: Percentage of variants that occur in both samples - not considering the genotype (only in VCF mode).
 * correlation: Correlation of variant genotypes.
 * ibs0: Percentage of variants with zero IBD, e.g. AA and CC (only in BAM mode).
 * ibs2: Percentage of variants with complete IBD, e.g. AA and AA.

Absolute cutoffs values for for same sample, parent-child pairs etc. do not exist, since they depend on ehtnicity, enrichment kit, etc.   
Example data for exome data (Agilent SureSelect Human All Exon V6) are shown here:

![alt text](qc_sample_correlation_ssHAEv6.png)






--

[back to main page](index.md)








