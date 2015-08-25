### BedLowCoverage tool help
	BedLowCoverage (0.1-52-g9f9161f)
	
	Detects low-coverage regions from a BAM file.
	
	Mandatory parameters:
	  -bam <file>     Input BAM file.
	  -cutoff <int>   Minimum depth to consider a base 'high coverage'.
	
	Optional parameters:
	  -in <file>      Input BED file containing the regions of interest. If unset, reads from STDIN.
	                  Default value: ''
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	  -min_mapq <int> Minimum mapping quality to consider a read.
	                  Default value: '1'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --tdx           Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)