### BamDownsample tool help
	BamDownsample (2020_03-159-g5c8b2e82)
	
	Downsamples a BAM file to the given percentage of reads.
	
	Mandatory parameters:
	  -in <file>          Input BAM file.
	  -percentage <float> Percentage of reads to keep.
	  -out <file>         Output BAM file.
	
	Optional parameters:
	  -test               Test mode: fix random number generator seed and write kept read names to STDOUT.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamDownsample changelog
	BamDownsample 2020_03-159-g5c8b2e82
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)