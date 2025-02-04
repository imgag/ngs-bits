### TsvMerge tool help
	TsvMerge (2024_08-113-g94a3b440)
	
	Merges TSV file based on a list of columns.
	
	Mandatory parameters:
	  -in <filelist>    Input TSV files that are merged. If only one file is given, each line in this file is interpreted as an input file path.
	  -cols <string>    Comma-separated list of column names used as key for merging.
	
	Optional parameters:
	  -out <file>       Output file. If unset, writes to STDOUT.
	                    Default value: ''
	  -numeric          If set, column names are interpreted as 1-based column numbers.
	                    Default value: 'false'
	  -simple           Fast and memory-efficient mode for merging files that are ordered in the same way and have no missing lines.
	                    Default value: 'false'
	  -mv <string>      Missing value, i.e. value that is inserted when key is missing in a file.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TsvMerge changelog
	TsvMerge 2024_08-113-g94a3b440
	
	2018-12-05 Added 'simple' mode.
[back to ngs-bits](https://github.com/imgag/ngs-bits)