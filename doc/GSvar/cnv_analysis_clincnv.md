## CNV analysis for exomes/genomes 


ClinCNV is a copy-number calling algorithm designed for exome and genome sequencing, which is based on the depth of coverage of adjacent regions.  
In exome sequencing, exon target regions alternate with non-target intron regions.  
In genome sequencing, a region is defined as a 1000 bases windows of the genomes.

For each regions, the expected depth of coveraged is compared with the observed depth of coverage and a log-likelihood of a copy-number alteration is gived for each region. 

### CNV analysis quality

Visualization and filtering of CNVs is done in the "Copy-number variants" dialog, which is shown below.
The dialog is opened from the main tool bar (0). 

In the upper part of the dialog (1), sample-specific information about the CNV calling is shown. It gives an impression  of the sample quality:

* *number of iterations:* If more CNVs than expected are encountered for a sample, the algorithm sensitivity is decreased and the algorithm is re-applied until a reasonable number of CNVs is called. The number of iterations counts how often the algorithm was started. Normally, this should be 1. A higher number idicates a problem during the analysis of the sample.
* *gender of sample*
* *was it outlier after clusting:* The input samples are clustered to find sample groups with similar depth profiles. If a samples is an outlier in this initial clustering, this indicates a problem during the analysis of the sample.
* *fraction of outliers:* Fraction of regions that are outliers after normalization

### CNV filtering

Above the CNV list, there are several options for filtering CNVs (2):

* size
* number of regions
* log-likelihood
* copy-number state
* allele frequency (estimated frequency of CNV for the region in the analyzed cohort) 
* target region (if set in the main filter panel for variants)
* genes (if set in the main filter panel for variants)
* phenotypes (if set in the main filter panel for variants)
* text (if set in the main filter panel for variants)
* generic annotation columns

For each CNV the following properties are shown (3):

* genomic position
* affected genes (if they are annotated)
* size
* copy-number change
* log-likelyhood of the copy-number change (bigger is better)
* number of regions
* frequency of the copy-number change in the analyzed cohort
* q-value (p-value corrected for the number of tests performed, smaller is better)

Additionally, generic annotation columns can be added (4), e.g.:

* overlap with copy-number polymorphism regions (as defined by [Zarrei et. al. 2015](http://www.nature.com/nrg/journal/v16/n3/abs/nrg3871.html))
* overlap with copy-number polymorphism regions (as defined by [Demidov](TODO))
* dosage-sensitive disease genes (from [Zarrei et. al. 2015](http://www.nature.com/nrg/journal/v16/n3/abs/nrg3871.html))
* OMIM genes

![alt text](cnv_filtering_clincnv.png)

More information about a copy-number variant can be found through the resources linked in the context menu (5). 


### Visualizing copy-number data in IGV

*Double-clicking* a CNV in the dialog, opens the CNV region in IGV (see also section [IGV integration](igv_integration.md)).

If the sample folder contains a copy-number SEG file, this file can be shown as a track in IGV. The default visualization is in points style. In this style gains are shown in blue and losses are shown in red.

In addition to the log-likelihood which is vizualized, the CNV track also contains TODO, which are shown as a tooltip when hovering over a region. In this screenshot a homozygous deletion of two exons in the OR2T10 gene is shown:


![alt text](cnv_visualization_clincnv.png)

We can adapt the IGV settings of the BAM track to visualize the breakpoints created by a copy-number variant (if they lie inside the target region):

* Open perferences dialog `View > Preferences` and on the `Alignments` tab, enable `Show soft-clipped bases`.
* Right-click on the BAM track and enable `View as pairs`.
* Right-click on the BAM track and enable `Color alignments by > Insert size and pair orientation`. 

Then, it is clear that in this case there is a tandem duplication of the region from exon 1 to exon 3.

![alt text](cnv_visualization2.png)

### Further reading

More details on CNV calling with ClinCNV can be found in the [ClinCNV documentation](https://github.com/imgag/ClinCNV/blob/master/doc/germline_CNV_analysis.md).

--

[back to main CNV page](cnv_analysis.md)





























