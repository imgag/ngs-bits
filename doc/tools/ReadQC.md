### ReadQC tool help
	ReadQC (0.1-184-gc4d2f1b)
	
	Calculates QC metrics on unprocessed paired-end reads (same number of cycles/reads).
	
	Mandatory parameters:
	  -in1 <file> Forward reads FASTQ file (gzipped or plain).
	
	Optional parameters:
	  -in2 <file> Reverse reads FASTQ file (gzipped or plain).
	              Default value: ''
	  -out <file> Output qcML file. If unset, writes to STDOUT.
	              Default value: ''
	  -txt        Writes TXT format instead of qcML.
	              Default value: 'false'
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)