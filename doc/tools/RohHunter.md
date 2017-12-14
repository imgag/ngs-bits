### RohHunter tool help
	RohHunter (0.1-936-g9573691)
	
	ROH detection based on a variant list annotated with AF values.
	
	Mandatory parameters:
	  -in <file>               Input variant list in VCF or GSvar format.
	  -out <file>              Output TSV file with ROH regions.
	
	Optional parameters:
	  -annotate <filelist>     List of BED files used for annotation. Each file adds a column to the output file. The base filename is used as colum name and 4th column of the BED file is used as annotation value.
	                           Default value: ''
	  -var_min_dp <int>        Minimum variant depth ('DP'). Variants with lower depth are excluded from the analysis.
	                           Default value: '20'
	  -var_min_q <float>       Minimum variant quality. Variants with lower depth are excluded from the analysis.
	                           Default value: '30'
	  -var_af_keys <string>    Annotation keys of allele frequency values (comma-separated).
	                           Default value: 'GNOMAD_AF,T1000GP_AF,EXAC_AF'
	  -roh_min_q <float>       Minimum Q score of ROH regions.
	                           Default value: '30'
	  -roh_min_markers <int>   Minimum marker count of ROH regions.
	                           Default value: '20'
	  -roh_min_size <float>    Minimum size in Kb of ROH regions.
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
	RohHunter 0.1-936-g9573691
	
	2017-12-07 Added generic annotation feature.
	2017-11-29 Added 'inc_chrx' flag.
	2017-11-21 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)