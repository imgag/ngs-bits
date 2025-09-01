### RohHunter tool help
	RohHunter (2025_07-53-gefb5888f)
	
	ROH detection based on a variant list.
	
	Runs of homozygosity (ROH) are detected based on the genotype annotations in the VCF file.Based on the allele frequency of the contained variants, each ROH is assigned an estimated likelyhood to be observed by chance (Q score).
	
	Mandatory parameters:
	  -in <file>                Input variant list in VCF format.
	  -out <file>               Output TSV file with ROH regions.
	
	Optional parameters:
	  -annotate <filelist>      List of BED files used for annotation. Each file adds a column to the output file. The base filename is used as column name and 4th column of the BED file is used as annotation value.
	                            Default value: ''
	  -exclude <file>           BED files with regions to exclude from ROH analysis. Regions where variant calling is not possible should be removed (centromers, MQ=0 regions and large stretches of N bases).
	                            Default value: ''
	  -var_min_dp <int>         Minimum variant depth ('DP'). Variants with lower depth are excluded from the analysis.
	                            Default value: '20'
	  -var_min_q <float>        Minimum variant quality. Variants with lower quality are excluded from the analysis.
	                            Default value: '20'
	  -var_af_keys <string>     Comma-separated allele frequency info field names in 'in'.
	                            Default value: ''
	  -var_af_keys_vep <string> Comma-separated VEP CSQ field names of allele frequency annotations in 'in'.
	                            Default value: ''
	  -roh_min_q <float>        Minimum Q score of output ROH regions.
	                            Default value: '30'
	  -roh_min_markers <int>    Minimum marker count of output ROH regions.
	                            Default value: '20'
	  -roh_min_size <float>     Minimum size in Kb of output ROH regions.
	                            Default value: '20'
	  -ext_marker_perc <float>  Percentage of ROH markers that can be spanned when merging ROH regions.
	                            Default value: '1'
	  -ext_size_perc <float>    Percentage of ROH size that can be spanned when merging ROH regions.
	                            Default value: '50'
	  -ext_max_het_perc <float> Maximum percentage of heterozygous markers in ROH regions.
	                            Default value: '5'
	  -inc_chrx                 Include chrX into the analysis. Excluded by default.
	                            Default value: 'false'
	  -debug                    Enable debug output
	                            Default value: 'false'
	
	Special parameters:
	  --help                    Shows this help and exits.
	  --version                 Prints version and exits.
	  --changelog               Prints changeloge and exits.
	  --tdx                     Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]         Settings override file (no other settings files are used).
	
### RohHunter changelog
	RohHunter 2025_07-53-gefb5888f
	
	2025-02-25 Added new parameters 'exclude', 'ext_max_het_perc' and 'debug'.
	2020-08-07 VCF files only as input format for variant list.
	2019-11-21 Added support for parsing AF data from any VCF info field (removed 'af_source' parameter).
	2019-03-12 Added support for input variant lists that are not annotated with VEP. See 'af_source' parameter.
	2018-09-12 Now supports VEP CSQ annotations (no longer support SnpEff ANN annotations).
	2017-12-07 Added generic annotation feature.
	2017-11-29 Added 'inc_chrx' flag.
	2017-11-21 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)