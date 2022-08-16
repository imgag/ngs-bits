### BedCoverage tool help
	BedCoverage (2022_07-52-g9fafa140)
	
	Annotates a BED file with the average coverage of the regions from one or several BAM/CRAM file(s).
	
	Mandatory parameters:
	  -bam <filelist> Input BAM/CRAM file(s).
	
	Optional parameters:
	  -min_mapq <int> Minimum mapping quality.
	                  Default value: '1'
	  -dup            Include reads marked as duplicates.
	                  Default value: 'false'
	  -in <file>      Input BED file. If unset, reads from STDIN.
	                  Default value: ''
	  -decimals <int> Number of decimals used in output.
	                  Default value: '2'
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	  -ref <file>     Reference genome for CRAM support (mandatory if CRAM is used).
	                  Default value: ''
	  -clear          Clear previous annotation columns before annotating (starting from 4th column).
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedCoverage changelog
	BedCoverage 2022_07-52-g9fafa140
	
	2022-08-12 Added parameter to clear previous annotation columns.
	2022-08-09 Removed mode parameter (panel mode is always used now).
	2020-11-27 Added CRAM support.
	2017-06-02 Added 'dup' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)