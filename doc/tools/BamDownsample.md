### BamDownsample tool help
	BamDownsample (2020_09-79-gc6314b21)
	
	Downsamples a BAM file to the given percentage of reads.
	
	Mandatory parameters:
	  -in <file>          Input BAM/CRAM file.
	  -percentage <float> Percentage of reads to keep.
	  -out <file>         Output BAM/CRAM file.
	
	Optional parameters:
	  -test               Test mode: fix random number generator seed and write kept read names to STDOUT.
	                      Default value: 'false'
	  -ref <string>       Reference genome for CRAM support (mandatory if CRAM is used).
	                      Default value: ''
	  -write_cram         Writes a CRAM file as output.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamDownsample changelog
	BamDownsample 2020_09-79-gc6314b21
	
	2020-11-27 Added CRAM support.
[back to ngs-bits](https://github.com/imgag/ngs-bits)