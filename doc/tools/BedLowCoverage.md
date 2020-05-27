### BedLowCoverage tool help
	BedLowCoverage (2020_03-162-gba45d65a)
	
	Detects low-coverage regions from a BAM file.
	
	Note that only read start/end are used. Thus, deletions in the CIGAR string are treated as covered.
	
	Mandatory parameters:
	  -bam <file>      Input BAM file.
	  -cutoff <int>    Minimum depth to consider a base 'high coverage'.
	
	Optional parameters:
	  -in <file>       Input BED file containing the regions of interest. If unset, reads from STDIN.
	                   Default value: ''
	  -wgs             WGS mode without target region. Genome information is taken from the BAM file.
	                   Default value: 'false'
	  -out <file>      Output BED file. If unset, writes to STDOUT.
	                   Default value: ''
	  -min_mapq <int>  Minimum mapping quality to consider a read.
	                   Default value: '1'
	  -min_baseq <int> Minimum base quality to consider a base.
	                   Default value: '0'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --changelog      Prints changeloge and exits.
	  --tdx            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedLowCoverage changelog
	BedLowCoverage 2020_03-162-gba45d65a
	
	2020-05-26 Added parameter 'min_baseq'.
	2016-06-09 The BED line name of the input BED file is now passed on to the output BED file.
[back to ngs-bits](https://github.com/imgag/ngs-bits)