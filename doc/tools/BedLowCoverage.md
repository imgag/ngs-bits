### BedLowCoverage tool help
	BedLowCoverage (0.1-350-g2af2ea1)
	
	Detects low-coverage regions from a BAM file.
	
	Mandatory parameters:
	  -bam <file>     Input BAM file.
	  -cutoff <int>   Minimum depth to consider a base 'high coverage'.
	
	Optional parameters:
	  -in <file>      Input BED file containing the regions of interest. If unset, reads from STDIN.
	                  Default value: ''
	  -wgs            WGS mode without target region. Genome information is taken from the BAM file.
	                  Default value: 'false'
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	  -min_mapq <int> Minimum mapping quality to consider a read.
	                  Default value: '1'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedLowCoverage changelog
	BedLowCoverage 0.1-350-g2af2ea1
	
	2016-06-09 The BED line name if the input BED file is now passed on to the output BED file.
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)