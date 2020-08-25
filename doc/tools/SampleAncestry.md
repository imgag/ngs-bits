### SampleAncestry tool help
	SampleAncestry (2020_03-260-ge35d12de)
	
	Estimates the ancestry of a sample based on variants.
	
	Mandatory parameters:
	  -in <filelist>    Input variant list(s) in VCF format.
	
	Optional parameters:
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -min_snps <int>   Minimum number of informative SNPs for population determination. If less SNPs are found, 'NOT_ENOUGH_SNPS' is returned.
	                    Default value: '1000'
	  -pop_dist <float> Minimum relative distance between first/second population score. If below this score 'ADMIXED/UNKNOWN' is called.
	                    Default value: '0.15'
	  -build <enum>     Genome build used to generate the input.
	                    Default value: 'hg19'
	                    Valid: 'hg19,hg38'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SampleAncestry changelog
	SampleAncestry 2020_03-260-ge35d12de
	
	2020-08-07 VCF files only as input format for variant list.
	2018-12-10 Fixed bug in handling of 'pop_dist' parameter.
	2018-07-11 Added build switch for hg38 support.
	2018-07-03 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)