### BamFilter tool help
	BamFilter (2018_06-21-g04d1d5e)
	
	Filter alignments in BAM file (no input sorting required).
	
	Mandatory parameters:
	  -in <file>    Input BAM file.
	  -out <file>   Output BAM file.
	
	Optional parameters:
	  -minMQ <int>  Minimum mapping quality.
	                Default value: '30'
	  -maxMM <int>  Maximum number of mismatches in aligned read, -1 to disable.
	                Default value: '4'
	  -maxGap <int> Maximum number of gaps (indels) in aligned read, -1 to disable.
	                Default value: '1'
	  -minDup <int> Minimum number of duplicates.
	                Default value: '0'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --changelog   Prints changeloge and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamFilter changelog
	BamFilter 2018_06-21-g04d1d5e
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)