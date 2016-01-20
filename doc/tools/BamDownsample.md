### BamDownsample tool help
	BamDownsample (0.1-184-gc4d2f1b)
	
	Downsamples a given BAM file to the given percentage of reads.
	
	Mandatory parameters:
	  -in <file>        Input BAM file.
	  -percentage <int> Percentage of reads from the input to the output file.
	  -out <file>       Output BAM file.
	
	Optional parameters:
	  -test             fixed seed for random and text output
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --tdx             Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)