### BamDownsample tool help
	BamDownsample (2018_03-2-g208f066)
	
	Downsamples a BAM file to the given percentage of reads.
	
	Mandatory parameters:
	  -in <file>        Input BAM file.
	  -percentage <int> Percentage of reads to keep.
	  -out <file>       Output BAM file.
	
	Optional parameters:
	  -test             Test mode: fix random number generator seed and write kept read names to STDOUT.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamDownsample changelog
	BamDownsample 2018_03-2-g208f066
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)