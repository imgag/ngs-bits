### TsvMerge tool help
	TsvMerge (0.1-827-g472ac9f)
	
	Merges TSV file based on a list of columns.
	
	Mandatory parameters:
	  -in <filelist> Input TSV files that are merged.
	  -cols <string> Comma-separated list of column names used as key for merging.
	
	Optional parameters:
	  -out <file>    Output file. If unset, writes to STDOUT.
	                 Default value: ''
	  -numeric       If set, column names are interpreted as 1-based column numbers.
	                 Default value: 'false'
	  -mv <string>   Missing value, i.e. value that is inserted when key is missing in a file.
	                 Default value: ''
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### TsvMerge changelog
	TsvMerge 0.1-827-g472ac9f
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)