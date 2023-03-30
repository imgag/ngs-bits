## Sample quality control

### Assessing quality by NGS QC metrics

To open the processed sample tabs of the all samples contained in the variant list use the main tool bar:

![alt text](qc1.png)

By default, it shows the most important quality metrics. All quality metrics can be shown using the `all` checkbox in the upper right corner.  
Details about the metrics can be shown in the tooltip of the metrics names.  

![alt text](qc2.png)

For some of the quality metrics, fixed default cutoffs are applied and values exceeding these scores are marked orange/red:

- target region read depth
- target region 20x percentage
- known variants percentage
- SNV allele frequency deviation

The colors do not necessarily mean that that sample is bad, because the scores depend on the processing system.  
Make sure to have a look at the value distribution of the metric in other samples (see plotting section).

The following table gives additional information for some quality metrics to make them easier to interprete:
<table>
	<tr><th>metric</th><th>value range</th><th>interpretation</th></tr>
	<tr><td>known variants percentage</td><td>This metric should be above 98%.</td><td></td></tr>
	<tr><td>transition/transversion ratio</td><td>For exomes, this metric should in the range 2.6-2.7.<br>For genomes it should be between 1.0 and 2.0.</td><td></td></tr>
	<tr><td>mapped read percentage</td><td>This metric should be above 98%.</td><td></td></tr>
	<tr><td>properly-paired read percentage</td><td>This metric should above 95%.</td><td></td></tr>
	<tr><td>target region 20x percentage</td><td>This metric should above 95% in most samples.</td><td>If this value is too low, the enrichment step did not work properly.</td></tr>
	<tr><td>SNV allele frequency deviation</td><td>This value should be below 3% for most samples.</td><td>Values above 10% are normally caused by contamination.<br>However, low coverage of a sample can also increase this metric.</td></tr>
</table>

### Plotting quality scores

The processed sample tab shows QC metrics for a single sample only.  
To show the distribution of the metric over time, use the `Plot` context menu entry:

![alt text](qc_plot_metrics.png)

If two metrics are selected, a scatter plot of the two metris is shown.  
For example, a low 20x coverage is normally caused by a low sequencing depth.
The normal ratio between average depth and 20x coverage can be best seen in this plot:


![alt text](qc_plot_metrics2.png)


**Note:** Each sample is colored according to the *processed sample* quality. Thus, even samples with normal values for the currently displayed metric, might be colored red for bad quality.

### Checking sample contamination

The `SNV allele frequency deviation` score indicates sample contamination, by looking at the percentage of SNVs that fall outside the expected allele frequency windows around 0.0, 0.5 or 1.0. This score should not be used for tumor samples.

A histogram of the allele frequency can be shown using the button in the main tool bar.  
This example shows how it should look like for a high quality exome. 


![alt text](qc_af_deviation.png)

### Checking sample gender

The sample gender can be easily checked from GSvar:

![alt text](qc_sample_gender.png)

For exomes and panels that contain the SRY gene, please use the method based on the SRY gene.  
If you are not sure if your panel contains the SRY gene, you can check the target region using IGV.  

### Checking sample similarity

If you have related samples and want to check how similar they are, you can do this from GSvar as well:

![alt text](qc_sample_correlation1.png)

The method based on the GSvar file is fast and should be used for most cases.  
SampleSimilarity calculates several metrics that measure how similar two samples are:

 * overlap: Percentage of variants that occur in both samples - not considering the genotype (only in VCF/GSvar mode).
 * correlation: Genotype correlation of overlapping variants.
 * ibs0: Percentage of variants with zero IBS, e.g. AA and CC (only in BAM mode).
 * ibs2: Percentage of variants with complete IBS, e.g. AA and AA.

Absolute cutoffs values for for same sample, parent-child pairs etc. do not exist, since they depend on ethnicity, enrichment kit, etc.
Example data for exome data (Agilent SureSelect Human All Exon V6) are shown here:

![alt text](qc_sample_correlation_ssHAEv6.png)

More information on sample similarity and plots for additional processing systems can be found [here](https://github.com/imgag/ngs-bits/blob/master/doc/tools/SampleSimilarity/index.md).




--

[back to main page](index.md)
