### FastqFormat tool help
	FastqFormat (0.1-33-gfdb89ad)
	
	Determines the FastQ quality encoding format.
	
	Mandatory parameters:
	  -in <file>   Input FASTQ file (gzipped or plain).
	
	Optional parameters:
	  -out <file>  Output text file. If unset, writes to STDOUT.
	               Default value: ''
	  -reads <int> The number of reads to parse.
	               Default value: '10000'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --tdx        Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits]("https://github.com/marc-sturm/ngs-bits")