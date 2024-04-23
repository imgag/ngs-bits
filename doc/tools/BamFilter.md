### BamFilter tool help
	BamFilter (2024_02-42-g36bb2635)
	
	Filter alignments in BAM/CRAM file (no input sorting required).
	
	Mandatory parameters:
	  -in <file>    Input BAM/CRAM file.
	  -out <file>   Output BAM/CRAM file.
	
	Optional parameters:
	  -minMQ <int>  Minimum mapping quality.
	                Default value: '30'
	  -maxMM <int>  Maximum number of mismatches in aligned read, -1 to disable.
	                Default value: '4'
	  -maxGap <int> Maximum number of gaps (indels) in aligned read, -1 to disable.
	                Default value: '1'
	  -minDup <int> Minimum number of duplicates.
	                Default value: '0'
	  -maxIS <int>  Maximum insert size, -1 to disable.
	                Default value: '-1'
	  -ref <file>   Reference genome for CRAM support (mandatory if CRAM is used).
	                Default value: ''
	  -write_cram   Writes a CRAM file as output.
	                Default value: 'false'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --changelog   Prints changeloge and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamFilter changelog
	BamFilter 2024_02-42-g36bb2635
	
	2024-02-15 Added option to remove large fragments.
	2020-11-27 Added CRAM support.
[back to ngs-bits](https://github.com/imgag/ngs-bits)