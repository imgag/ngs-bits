### TsvSlice tool help
	TsvSlice (2024_08-113-g94a3b440)
	
	Extracts/reorders columns of a TSV file.
	
	Mandatory parameters:
	  -cols <string>    Comma-separated list of column names to extract.
	
	Optional parameters:
	  -in <file>        Input TSV file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output file. If unset, writes to STDOUT.
	                    Default value: ''
	  -numeric          If set, column names are interpreted as 1-based column numbers.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TsvSlice changelog
	TsvSlice 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)