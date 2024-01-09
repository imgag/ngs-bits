### BamDownsample tool help
	BamDownsample (2023_11-42-ga9d1687d)
	
	Downsamples a BAM file to the given percentage of reads.
	
	Mandatory parameters:
	  -in <file>          Input BAM/CRAM file.
	  -percentage <float> Percentage of reads to keep.
	  -out <file>         Output BAM/CRAM file.
	
	Optional parameters:
	  -test               Test mode: fix random number generator seed and write kept read names to STDOUT.
	                      Default value: 'false'
	  -ref <file>         Reference genome for CRAM support (mandatory if CRAM is used).
	                      Default value: ''
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamDownsample changelog
	BamDownsample 2023_11-42-ga9d1687d
	
	2020-11-27 Added CRAM support.
[back to ngs-bits](https://github.com/imgag/ngs-bits)