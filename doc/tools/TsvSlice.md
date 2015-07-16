### TsvSlice tool help
	TsvSlice (0.1-46-gb124721)
	
	Extracts/reorders columns of a TSV file.
	
	Mandatory parameters:
	  -cols <string> Comma-separated list of column names to extract.
	
	Optional parameters:
	  -in <file>     Input TSV file. If unset, reads from STDIN.
	                 Default value: ''
	  -out <file>    Output file. If unset, writes to STDOUT.
	                 Default value: ''
	  -numeric       If set, column names are interpreted as 1-based column numbers.
	                 Default value: 'false'
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --tdx          Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)