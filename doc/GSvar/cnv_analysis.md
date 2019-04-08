## CNV analysis

Copy-number variant (CNV) calling is performed based on the depth of coverage, i.e. the number of reads, in a defined region.  
For exome or panel sequencing, a region is typically an exon. For genome sequencing, a region is defined as a slice of the genome with a defined size, e.g. 1000 bases.

### General CNV analysis strategy

The analysis strategy for CNVs depends on inheritance mode and other factors.  
These are examples of ananlysis steps that are commonly performed:

1. Check for **compound heterozygous** variants: CNV/CNV und CNV/SNV
	
	**Note:** For the analysis of compound heterozygous CNV and SNVs, the filtering of the small variants is relevent: use the `dominant_relaxed` filter.

1. Check for **homozygous deletions** using the `copy number=0` filter
1. Check for **microdeletion syndromes** using `min regions=10` filter
1. Check for CNVs matching the **patient phenotype** (`target region` and/or `phenotypes` filter)
1. Check for CNVS in **ACMG** target-region

### CNV analysis details

Two different algorihtms are used for CNV calling, depending on the sequencing strategy.  
For exome and genome sequencing data, [ClinCNV](https://github.com/imgag/ClinCNV) is used for copy-number analysis.  
For panel seuqencing data, [CnvHunter](https://github.com/imgag/ngs-bits/) is used.

For detailled information, please have a look a the algorihtm-specific sections:

- [exome/genome CNVs](cnv_analysis_clincnv.md)
- [panel CNVs](cnv_analysis_cnvhunter.md)

## FAQ

### How do I re-start the CNV analysis of a sample

Copy-number variant calling is based on a virtual reference sample, which is constructed of the 20 most similar samples with the same processing system. Thus, at least 20 samples of the sample processing system are needed to perform the CNV analysis. The more samples there are, the more accurate the CNV analysis will be.

Thus, re-analyzing copy-number variants when more reference samples are sequenced is a common task. It can be performed using the sample details dock widget:

![alt text](cnv_reanalyze.png)

--

[back to main page](index.md)
























