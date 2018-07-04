### SampleAncestry tool help
	SampleAncestry (2018_06-3-g45da761)
	
	Estimates the ancestry of a sample based on variants.
	
	The ancestry estimation is based on a simple correlation to the most informative exonic SNPs for each population.
	A test against the PCA-based method implemented in Peddy (https://github.com/brentp/peddy) showed 95% overlap of estimates.
	
	Mandatory parameters:
	  -in <filelist>    Input variant list(s) in VCF format.
	
	Optional parameters:
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -min_snps <int>   Minimum number of informative SNPs for population determination. If less SNPs are found, 'NOT_ENOUGH_SNPS' is returned.
	                    Default value: '1000'
	  -pop_dist <float> Minimum relative distance between first/second population score. If below this score 'ADMIXED/UNKNOWN' is called.
	                    Default value: '0.15'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SampleAncestry changelog
	SampleAncestry 2018_06-3-g45da761
	
	2018-07-03 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)


