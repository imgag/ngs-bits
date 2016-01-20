### FastqTrim tool help
	FastqTrim (0.1-184-gc4d2f1b)
	
	Trims start/end bases from all reads in a FASTQ file.
	
	Mandatory parameters:
	  -in <file>   Input gzipped FASTQ file.
	  -out <file>  Output gzipped FASTQ file.
	
	Optional parameters:
	  -start <int> Number of bases to trim from start of the read.
	               Default value: '0'
	  -end <int>   Number of bases to trim from the end of the read.
	               Default value: '0'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --tdx        Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)