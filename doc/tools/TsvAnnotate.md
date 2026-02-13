### TsvAnnotate tool help
	TsvAnnotate (2025_12-104-g15d04bf0)
	
	Extends TSV file by appending columns from a second TSV file.
	
	Mandatory parameters:
	  -in2 <file>       Input TSV files that is used as source of annotated columns.
	  -c1 <string>      Column in 'in1' that is used for matching lines between files.
	  -anno <string>    Comma-separated column list from 'in2' that is appended to 'in1'. Order matters.
	
	Optional parameters:
	  -in1 <file>       Input TSV files that is annoated. If unset, reads from STDIN.
	                    Default value: ''
	  -c2 <string>      Column in 'in2' that is used for matching lines between files. If unset, the value of 'c1' is used.
	                    Default value: ''
	  -out <file>       Output file. If unset, writes to STDOUT.
	                    Default value: ''
	  -mv <string>      Missing value, i.e. value that is used when data is missing in 'in2'.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TsvAnnotate changelog
	TsvAnnotate 2025_12-104-g15d04bf0
	
	2026-01-20 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)