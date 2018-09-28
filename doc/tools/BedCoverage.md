### BedCoverage tool help
	BedCoverage (2018_06-59-g24102d3)
	
	Extracts the average coverage for input regions from one or several BAM file(s).
	
	Mandatory parameters:
	  -bam <filelist> Input BAM file(s).
	
	Optional parameters:
	  -min_mapq <int> Minimum mapping quality.
	                  Default value: '1'
	  -dup            Include reads marked as duplicates.
	                  Default value: 'false'
	  -in <file>      Input BED file (note that overlapping regions will be merged before processing). If unset, reads from STDIN.
	                  Default value: ''
	  -mode <enum>    Mode to optimize run time. Use 'panel' mode if only a small part of the data in the BAM file is accessed, e.g. a sub-panel of an exome.
	                  Default value: 'default'
	                  Valid: 'default,panel'
	  -decimals <int> Number of decimals used in output.
	                  Default value: '2'
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedCoverage changelog
	BedCoverage 2018_06-59-g24102d3
	
	2017-06-02 Added 'dup' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)