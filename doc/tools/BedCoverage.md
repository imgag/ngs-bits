### BedCoverage tool help
	BedCoverage (0.1-528-gaae18be)
	
	Extracts the average coverage for input regions from one or several BAM file(s).
	
	Mandatory parameters:
	  -bam <filelist> Input BAM file(s).
	
	Optional parameters:
	  -min_mapq <int> Minimum mapping quality.
	                  Default value: '1'
	  -in <file>      Input BED file (note that overlapping regions will be merged before processing). If unset, reads from STDIN.
	                  Default value: ''
	  -mode <enum>    Mode to optimize run time. Use 'panel' mode if only a small part of the data in the BAM file is processed, e.g. a sub-panel of an exome
	                  Default value: 'default'
	                  Valid: 'default,panel'
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedCoverage changelog
	BedCoverage 0.1-528-gaae18be
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)