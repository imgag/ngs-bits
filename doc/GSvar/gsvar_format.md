# GSvar format

The [megSAP analysis pipelines](https://github.com/imgag/megSAP/) create variant lists in 
**GSvar  format** in addition to the standard [VCF](https://de.wikipedia.org/wiki/VCF) format.

The GSvar format is a tab-separated text format.

## Format details

### Meta data header lines

Similar to VCF format, the GSvar format can contain meta data in header lines that start with `##`:

- The `##ANALYSISTYPE=` header specifies the analysis type. It can be present only once. Valid analysis types are:  
	`GERMLINE_SINGLESAMPLE`, `GERMLINE_MULTISAMPLE`,  `GERMLINE_TRIO`,  `SOMATIC_SINGLESAMPLE`,  `SOMATIC_PAIR`  

- The `##PIPELINE=` header specifies the analysis pipeline used to perform the analysis.
	
- The `##SAMPLE=` header specifies the analyzed sample name(s) and can contain additional annotations. It can occur several times, if several samples were analyzed.   

- The `##DESCRIPTION=` header gives a detailled description of acolumn. It can occur several times.    

- The `##FILTER=` header describes entries in the `filter` column. It can occur several times.  

 
### Variant lines

After the meta data header lines, the main header line and the variant lines follow:

<table>
	<tr><th>#chr</th><th>start</th><th>end</th><th>ref</th><th>obs</th><th>NA12878_03</th><th>filter</th><th>quality</th><th>gene</th><th>variant_type</th><th>coding_and_splicing</th><th>...</th></tr>
	<tr><td>chr1</td><td>27682481</td><td>27682481</td><td>G</td><td>A</td><td>het</td><td>off-target</td><td>QUAL=2185;DP=168;AF=0.51;MQM=60;SAP=15;ABP=3</td><td>MAP3K6</td><td>intron</td><td>MAP3K6:ENST00000357582:intron_variant:MODIFIER:intron27/28:c.3711+36C>T::,MAP3K6:ENST00000374040:intron_variant:MODIFIER:intron26/27:c.3687+36C>T::,MAP3K6:ENST00000493901:intron_variant:MODIFIER:intron28/29:c.3711+36C>T::</td><td>...</td></tr>
	<tr><td>chr1</td><td>62740781</td><td>62740781</td><td>A</td><td>-</td><td>het</td><td>off-target</td><td>QUAL=270;DP=116;AF=0.20;MQM=60;SAP=3;ABP=95</td><td>KANK4</td><td>intron</td><td>KANK4:ENST00000354381:intron_variant:MODIFIER:intron2/8:c.17-3520del::,KANK4:ENST00000371153:intron_variant:MODIFIER:intron2/9:c.17-22del::</td><td>...</td></tr>
	<tr><td>chr2</td><td>47635523</td><td>47635523</td><td>-</td><td>T</td><td>het</td><td>off-target</td><td>QUAL=53;DP=18;AF=0.33;MQM=60;SAP=16;ABP=7</td><td>MSH2</td><td>intron</td><td>MSH2:ENST00000233146:intron_variant:MODIFIER:intron1/15:c.212-4dup::,MSH2:ENST00000406134:intron_variant:MODIFIER:intron1/15:c.212-4dup::,MSH2:ENST00000543555:intron_variant:MODIFIER:intron2/16:c.14-4dup::</td><td>...</td></tr>
</table>

The main columns in the GSvar format are:

* *chr*: Chromosome the variant is located on.
* *start*: Start position of the variant on the chromosome. For insertions, the position of the base before the insertion is shown.
* *end*: End position of the variant on the chromosome. For insertions, the position of the base before the insertion is shown.
* *ref*: Reference bases in the reference genome at the variant position. `-` in case of an insertion.
* *obs*: Alternate bases observed in the sample. `-` in case of an deletion.
* *filter*: Filter columns with entries according to `##FILTER`headers.
* *quality*: Quality information about the variant:
	* *QUAL*: Phread-scaled probability that the variant is a true-positive.
	* *DP*: Sequencing depth at the variant locus.
	* *QD*: Quality divided by depth. This is a better filter for artefacts than the absolute quality.
	* *AF*: Allele frequency of the variant in the sample.
	* *MQM*: Phread-scaled mapping quality of the alternate bases
	* *SAP*: Phread-scaled likelyhood of observed alternate base strand bias, i.e. distribution on forward/reverse strand.
	* *ABP*: Phread-scaled likelyhood of observed allele balance, i.e. distribution of reference base vs. alternate base (only for heterozygous variants).

#### Gennotype column(s)

The `NA12878_03` in the example above is the genotype column for sample NA12878_03 in a single-sample germline analysis.  
It contains `het` for heterozygous variants, and `hom` for homzygous variants.

There can be several genotype columns of different processed sample in case a muli-sample or trio analysis was performed.  
In this case the genotype `wt` (wild-type) is used to indicate that the samples does not contain the variant.

In case of a tumor-only of tumor-normal analysis, no genotype column is provided.  
Tn this case the colums `tumor_af`, `tumor_dp`, `normal_af` and `normal_dp` are used.  
They contain the allele frequency and depth of tumor/normal sample.

#### Annotation columns

All other columns are annotation column, e.g.:

* *gene*: List of genes affected by the variants.
* *variant\_type*: Variant type accoring to [sequence ontology](http://www.sequenceontology.org/browser/current_release/term/SO:0001060).
* *coding\_and\_splicing*: cDNA/protein change caused by the variant.


## Variant filtering

There are several ways to filter variant lists in GSvar format to identify rare pathogenic variants:

1. The file can be filtered interactively using the **GSvar** application.
2. The file can be filtered on the Linux command line using the [VariantFilterAnnotations tool](https://github.com/imgag/ngs-bits/blob/master/doc/tools/VariantFilterAnnotations.md) of ngs-bits.
3. The file can be filtered in Microsoft Excel.

### Pre-filtering of WGS variant lists

In WGS the VCF file contain about 5 million variants, which is too much to put into a GSvar file.  
Thus only variants that match at least one of these criteria are included in the GSvar file:

	- VEP impact HIGH, MODERATE or LOW
	- annotated as pathogenic or likely pathogenic in ClinVar oder HGMD
	- annotated as class 4, 5 or M in NGSD
	- genomAD AF <= 2%
	- on chrMT

This means that common variants (AF>2%) that are intronic or intergenic are not included.


[back to the start page](../README.md)
