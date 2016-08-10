### TsvSlice tool help
	TsvSlice (0.1-420-g3536bb0)
	
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
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### TsvSlice changelog
	TsvSlice 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)