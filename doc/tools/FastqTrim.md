### FastqTrim tool help
	FastqTrim (0.1-420-g3536bb0)
	
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
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastqTrim changelog
	FastqTrim 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)