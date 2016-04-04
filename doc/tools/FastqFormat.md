### FastqFormat tool help
	FastqFormat (0.1-222-g9be2128)
	
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
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastqFormat changelog
	FastqFormat 0.1-222-g9be2128
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)