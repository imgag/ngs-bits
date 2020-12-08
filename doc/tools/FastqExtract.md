### FastqExtract tool help
	FastqExtract (2020_03-159-g5c8b2e82)
	
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
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastqExtract changelog
	FastqExtract 2020_03-159-g5c8b2e82
	
	2020-07-15 Added 'compression_level' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)