### FastqExtract tool help
	FastqExtract (2024_08-110-g317f43b9)
	
	Extracts reads from a FASTQ file according to an ID list. Trims the reads if lengths are given.
	
	Mandatory parameters:
	  -in <file>               Input FASTQ file (gzipped or plain).
	  -ids <file>              Input TSV file containing IDs (without the '@') in the first column and optional length in the second column.
	  -out <file>              Output FASTQ file.
	
	Optional parameters:
	  -v                       Invert match: keep non-matching reads.
	                           Default value: 'false'
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
	
### FastqExtract changelog
	FastqExtract 2024_08-110-g317f43b9
	
	2023-04-18 Added support for long reads.
	2020-07-15 Added 'compression_level' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)