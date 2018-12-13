### TsvMerge tool help
	TsvMerge (2018_11-39-g5d6be46)
	
	Merges TSV file based on a list of columns.
	
	Mandatory parameters:
	  -in <filelist> Input TSV files that are merged. If only one file is provided, the lines of the file are interpreted as file names.
	  -cols <string> Comma-separated list of column names used as key for merging.
	
	Optional parameters:
	  -out <file>    Output file. If unset, writes to STDOUT.
	                 Default value: ''
	  -numeric       If set, column names are interpreted as 1-based column numbers.
	                 Default value: 'false'
	  -simple        Fast and memty-efficient mode for merging files that are ordered in the same way and have no missing lines.
	                 Default value: 'false'
	  -mv <string>   Missing value, i.e. value that is inserted when key is missing in a file.
	                 Default value: ''
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### TsvMerge changelog
	TsvMerge 2018_11-39-g5d6be46
	
	2018-12-05 Added 'simple' mode.
[back to ngs-bits](https://github.com/imgag/ngs-bits)