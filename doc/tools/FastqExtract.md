### FastqExtract tool help
	FastqExtract (0.1-190-g94e4c3d)
	
	Extracts reads from a FASTQ file according to an ID list. Trims the reads if lengths are given.
	
	Mandatory parameters:
	  -in <file>  Input FASTQ file (gzipped or plain).
	  -ids <file> Input TSV file containing IDs (without the '@') in the first column and optional length in the second column.
	  -out <file> Output FASTQ file.
	
	Optional parameters:
	  -v          Invert match: keep non-matching reads.
	              Default value: 'false'
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)