### BamToFastq tool help
	BamToFastq (2019_09-51-g84b6c695)
	
	Converts a coordinate-sorted BAM file to FASTQ files (paired-end only).
	
	Mandatory parameters:
	  -in <file>          Input BAM file.
	  -out1 <file>        Read 1 output FASTQ.GZ file.
	  -out2 <file>        Read 2 output FASTQ.GZ file.
	
	Optional parameters:
	  -reg <string>       Export only reads in the given region. Format: chr:start-end.
	                      Default value: ''
	  -remove_duplicates  Does not export duplicate reads into the FASTQ file.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamToFastq changelog
	BamToFastq 2019_09-51-g84b6c695
	
	2020-03-21 Added 'reg' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)