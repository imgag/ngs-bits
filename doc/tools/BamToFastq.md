### BamToFastq tool help
	BamToFastq (2020_09-79-gc6314b21)
	
	Converts a coordinate-sorted BAM file to FASTQ files (paired-end only).
	
	Mandatory parameters:
	  -in <file>               Input BAM/CRAM file.
	  -out1 <file>             Read 1 output FASTQ.GZ file.
	  -out2 <file>             Read 2 output FASTQ.GZ file.
	
	Optional parameters:
	  -reg <string>            Export only reads in the given region. Format: chr:start-end.
	                           Default value: ''
	  -remove_duplicates       Does not export duplicate reads into the FASTQ file.
	                           Default value: 'false'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	  -write_buffer_size <int> Output write buffer size (number of FASTQ entry pairs).
	                           Default value: '100'
	  -ref <string>            Reference genome for CRAM support (mandatory if CRAM is used).
	                           Default value: ''
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamToFastq changelog
	BamToFastq 2020_09-79-gc6314b21
	
	2020-11-27 Added CRAM support.
	2020-05-29 Massive speed-up by writing in background. Added 'compression_level' parameter.
	2020-03-21 Added 'reg' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)