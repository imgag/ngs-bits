### FastqTrim tool help
	FastqTrim (2025_05-79-g6c060cfd)
	
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
	  -max_len <int>           Only trim reads smaller than the given length. Used e.g. to remove UMIs at the read end from read-throughs.
	                           Default value: '0'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	  -long_read               Support long reads (> 1kb).
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### FastqTrim changelog
	FastqTrim 2025_05-79-g6c060cfd
	
	2023-06-15 Added support for long reads.
	2020-07-15 Added 'compression_level' parameter.
	2016-08-26 Added 'len' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)