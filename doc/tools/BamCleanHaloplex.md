### BamCleanHaloplex tool help
	BamCleanHaloplex (2020_09-90-g55257954)
	
	BAM cleaning for Haloplex.
	
	Mandatory parameters:
	  -in <file>       Input BAM/CRAM file.
	  -out <file>      Output BAM/CRAM file.
	
	Optional parameters:
	  -min_match <int> Minimum number of CIGAR matches (M).
	                   Default value: '30'
	  -ref <string>    Reference genome for CRAM compression (compulsory for CRAM support).
	                   Default value: ''
	  -write_cram      Writes a CRAM file as output.
	                   Default value: 'false'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --changelog      Prints changeloge and exits.
	  --tdx            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamCleanHaloplex changelog
	BamCleanHaloplex 2020_09-90-g55257954
	
	2020-11-27 Added CRAM support.
[back to ngs-bits](https://github.com/imgag/ngs-bits)