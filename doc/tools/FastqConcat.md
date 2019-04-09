### FastqConcat tool help
	FastqConcat (2019_03-42-gfa81c47)
	
	Concatinates several FASTQ files into one output FASTQ file.
	
	Mandatory parameters:
	  -in <filelist>           Input (gzipped) FASTQ files.
	  -out <file>              Output gzipped FASTQ file.
	
	Optional parameters:
	  -compression_level <int> gzip compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastqConcat changelog
	FastqConcat 2019_03-42-gfa81c47
	
	2019-04-08 Initial version of this tool
[back to ngs-bits](https://github.com/imgag/ngs-bits)