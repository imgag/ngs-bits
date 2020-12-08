### BedReadCount tool help
	BedReadCount (2020_09-79-gc6314b21)
	
	Annotates the regions in a BED file with the read count from a BAM/CRAM file.
	
	Mandatory parameters:
	  -bam <file>     Input BAM/CRAM file.
	
	Optional parameters:
	  -min_mapq <int> Minimum mapping quality.
	                  Default value: '1'
	  -in <file>      Input BED file (note that overlapping regions will be merged before processing). If unset, reads from STDIN.
	                  Default value: ''
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	  -ref <string>   Reference genome for CRAM support (mandatory if CRAM is used).
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedReadCount changelog
	BedReadCount 2020_09-79-gc6314b21
	
	2020-11-27 Added CRAM support.
[back to ngs-bits](https://github.com/imgag/ngs-bits)