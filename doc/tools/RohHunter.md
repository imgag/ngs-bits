### RohHunter tool help
	RohHunter (2019_03-10-gecc0f8e)
	
	ROH detection based on a variant list.
	
	Runs of homozygosity (ROH) are detected based on the genotype annotations in the VCF file.Based on the allele frequency of the contained variants, each ROH is assigned an estimated likelyhood to be observed by chance (Q score).
	
	Mandatory parameters:
	  -in <file>               Input variant list in VCF or GSvar format.
	  -out <file>              Output TSV file with ROH regions.
	
	Optional parameters:
	  -annotate <filelist>     List of BED files used for annotation. Each file adds a column to the output file. The base filename is used as colum name and 4th column of the BED file is used as annotation value.
	                           Default value: ''
	  -var_min_dp <int>        Minimum variant depth ('DP'). Variants with lower depth are excluded from the analysis.
	                           Default value: '20'
	  -var_min_q <float>       Minimum variant quality. Variants with lower quality are excluded from the analysis.
	                           Default value: '30'
	  -var_af_keys <string>    Comma-separated field names of allele frequency values in input file VEP annotation or 'af_source'.
	                           Default value: 'gnomAD_AF,AF'
	  -af_source <file>        Tabix-indexed VCF file used as source for allele frequency data. If unset, it is assumed that the input variant list is annotated using Ensembl VEP.
	                           Default value: ''
	  -roh_min_q <float>       Minimum Q score of output ROH regions.
	                           Default value: '30'
	  -roh_min_markers <int>   Minimum marker count of output ROH regions.
	                           Default value: '20'
	  -roh_min_size <float>    Minimum size in Kb of output ROH regions.
	                           Default value: '20'
	  -ext_marker_perc <float> Percentage of ROH markers that can be spanned when merging ROH regions .
	                           Default value: '1'
	  -ext_size_perc <float>   Percentage of ROH size that can be spanned when merging ROH regions.
	                           Default value: '50'
	  -inc_chrx                Include chrX into the analysis. Excluded by default.
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### RohHunter changelog
	RohHunter 2019_03-10-gecc0f8e
	
	2019-03-12 Added support for input variant lists that are not annotated with VEP. See 'af_source' parameter.
	2018-09-12 Now supports VEP CSQ annotations (no longer support SnpEff ANN annotations).
	2017-12-07 Added generic annotation feature.
	2017-11-29 Added 'inc_chrx' flag.
	2017-11-21 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)