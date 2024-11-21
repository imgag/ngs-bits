### FastqConvert tool help
	FastqConvert (2024_08-110-g317f43b9)
	
	Converts the quality scores from Illumina 1.5 offset to Sanger/Illumina 1.8 offset.
	
	Mandatory parameters:
	  -in <file>               Input gzipped FASTQ file.
	  -out <file>              Output gzipped FASTQ file.
	
	Optional parameters:
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### FastqConvert changelog
	FastqConvert 2024_08-110-g317f43b9
	
	2020-07-15 Added 'compression_level' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)