# Data analysis

## Where can I trigger analysis jobs?

You can trigger the (re-)analysis of the processed samples from several places in GSvar.

###Analysis status tab

The `analysis status` tab is opened from the GSvar tool bar (![alt text](analysis_status.png)):

![alt text](analysis_status_widget.png)

Here you see all analysis jobs and their current status.

In the upper right corner of the tab, you find buttons to queue the analysis of:

- single sample analysis
- trios analysis
- multi-sample analysis
- somatic analysis


### Processed sample tab

In the upper part of the `processed sample` tab, you can (re-)start the single-sample analysis of the processed sample using the button (![alt text](analysis_restart.png)).  

### Main tool bar

In the main tool bar, you can restart the analysis of the currently open GSvar file using the button (![alt text](analysis_restart.png)).  
This works for all analysis types:

- single sample analysis
- trios analysis
- multi-sample analysis
- somatic analysis

### Sample search

The `sample search` can be opened via the main menu bar of GSvar (![alt text](sample_search.png)).  
Here processed samples can be searched for, e.g. of a project, run or processing system.  
Through the context menu of the search results, a batch of samples can be analyzed.


## What do the analysis steps mean and what do they do?

Here is a summary of the analysis steps and a description what they do:

<table border=1>
<tr>
	<th>analysis type - step</th>
	<th>description</th>
	<th>produced files</th>
</tr>
<tr>
	<td>single sample - ma</td>
	<td>mapping of reads to the reference genome.</td>
	<td>BAM file</td>
</tr>
<tr>
	<td>ingle sample - vc</td>
	<td>Variant calling of small variants (SNVs and small indels).<br>Annotation of the variants with information from several databases and tools.</td>
	<td>VCF file, GSvar file</td>
</tr>
<tr>
	<td>ingle sample - cn</td>
	<td>Variant calling of CNVs based on depth of coverage using other samples are reference.<br>Annotation of the variants with information from several databases and tools.</td>
	<td>CNV file (TSV format)</td>
</tr>
<tr>
	<td>ingle sample - sv </td>
	<td>Variant calling of structural variants based on read data (split reads, soft-clipped reads, not properly paired reads).<br>Annotation of the variants with information from several databases and tools.</td>
	<td>SV file (BEDPE format)</td>
</tr>
<tr>
	<td>all - db</td>
	<td>Import of QC data and variant data into the NGSD database.</td>
	<td></td>
</tr>
</table>


## What is the difference between re-annotation and a normal analysis.

When performing reannotation (e.g. by checking the box `annotate only` in the single-sample analysis dialog), only variant annotation is performed.  
Variant calling is skipped and the annotations of the variants is updated.  
This is usually performed to update annotations as the data in public databases (ClinVar, HGMD, OMIM, ...) is updated regularly.

--

[back to main page](index.md)
