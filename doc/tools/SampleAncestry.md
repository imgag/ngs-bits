### SampleAncestry tool help
	SampleAncestry (2018_06-3-g45da761)
	
	Estimates the ancestry of a sample based on variants.
	
	The ancestry estimation is based on correlating the sample variants with population-specific SNPs.
	For each population (AFR,EUR,SAS,EAS) the 1000 most informative exonic SNPs were selected.  
	A benchmark on the 1000 Genomes variant data assigned 99.9% of the samples to the right population (2155 of 2157).
	
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



