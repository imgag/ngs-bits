### BamHighCoverage tool help
	BamHighCoverage (2018_04-32-g853eeef)
	
	Determines high-coverage regions in a BAM file.
	
	Mandatory parameters:
	  -in <file>      Input BAM file.
	  -cutoff <int>   Minimum depth to consider a chromosomal position 'high coverage'.
	
	Optional parameters:
	  -min_mapq <int> Minimum mapping quality.
	                  Default value: '1'
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamHighCoverage changelog
	BamHighCoverage 2018_04-32-g853eeef
	
	2018-05-03 Initial version of the tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)