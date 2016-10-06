### ReadQC tool help
	ReadQC (0.1-487-g4a3dc62)
	
	Calculates QC metrics on unprocessed NGS reads.
	
	Mandatory parameters:
	  -in1 <filelist> Forward input gzipped FASTQ file(s).
	
	Optional parameters:
	  -in2 <filelist> Reverse input gzipped FASTQ file(s) for paired-end mode (same number of cycles/reads as 'in1').
	                  Default value: ''
	  -out <file>     Output qcML file. If unset, writes to STDOUT.
	                  Default value: ''
	  -txt            Writes TXT format instead of qcML.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### ReadQC changelog
	ReadQC 0.1-487-g4a3dc62
	
	2016-08-19 Added support for multiple input files.
[back to ngs-bits](https://github.com/imgag/ngs-bits)