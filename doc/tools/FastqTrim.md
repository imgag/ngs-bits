### FastqTrim tool help
	FastqTrim (2020_03-159-g5c8b2e82)
	
	Trims start/end bases from all reads in a FASTQ file.
	
	Mandatory parameters:
	  -in <file>               Input gzipped FASTQ file.
	  -out <file>              Output gzipped FASTQ file.
	
	Optional parameters:
	  -start <int>             Trim this number of bases from the start of the read.
	                           Default value: '0'
	  -end <int>               Trim this number of bases from the end of the read.
	                           Default value: '0'
	  -len <int>               Restrict read length to this value (after trimming from start/end).
	                           Default value: '0'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastqTrim changelog
	FastqTrim 2020_03-159-g5c8b2e82
	
	2020-07-15 Added 'compression_level' parameter.
	2016-08-26 Added 'len' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)