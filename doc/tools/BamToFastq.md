### BamToFastq tool help
	BamToFastq (2019_09-55-g605f04ed)
	
	Converts a coordinate-sorted BAM file to FASTQ files (paired-end only).
	
	Mandatory parameters:
	  -in <file>          Input BAM file.
	  -out1 <file>        Read 1 output FASTQ.GZ file.
	  -out2 <file>        Read 2 output FASTQ.GZ file.
	
	Optional parameters:
	  -remove_duplicates  Does not export duplicate reads into the FASTQ file.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamToFastq changelog
	BamToFastq 2019_09-55-g605f04ed
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)