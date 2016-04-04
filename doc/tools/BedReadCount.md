### BedReadCount tool help
	BedReadCount (0.1-222-g9be2128)
	
	Annotates the regions in a BED file with the read count from a BAM file.
	
	Mandatory parameters:
	  -bam <file>     Input BAM file.
	
	Optional parameters:
	  -min_mapq <int> Minimum mapping quality.
	                  Default value: '1'
	  -in <file>      Input BED file (note that overlapping regions will be merged before processing). If unset, reads from STDIN.
	                  Default value: ''
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedReadCount changelog
	BedReadCount 0.1-222-g9be2128
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)