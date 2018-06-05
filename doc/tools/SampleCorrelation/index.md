# SampleCorrelation documentation

SampleCorrelation calculates two metrics that express how related two samples are:

 * overlap: Percentage of variants that occur in both samples (not considering the genotype).
 * correlation: Correlation of the genotypes based on overlapping variants.

Absolute cutoffs values for for same sample, parent-child pairs etc. do not exist, since they depend on the enrichment kit that was used.   
Example data for exome data (Agilent SureSelect Human All Exon V6) are shown here:
![sample correlation image](ssHAEv6_sample_correlation.png) 

## Help and ChangeLog

The CnvHunter command-line help and changelog can be found [here](../SampleCorrelation.md).

[back to ngs-bits](https://github.com/imgag/ngs-bits)






