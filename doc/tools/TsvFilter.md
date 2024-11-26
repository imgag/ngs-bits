### TsvFilter tool help
	TsvFilter (2024_08-113-g94a3b440)
	
	Filters the rows of a TSV file according to the value of a specific column.
	
	Mandatory parameters:
	  -filter <string>  Filter string with column name, operation and value,e.g. 'depth > 17'.
	Valid operations are '>','>=','=','<=','<','is','contains'.
	
	Optional parameters:
	  -in <file>        Input TSV file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -numeric          If set, column name is interpreted as a 1-based column number.
	                    Default value: 'false'
	  -v                Invert filter.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TsvFilter changelog
	TsvFilter 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)