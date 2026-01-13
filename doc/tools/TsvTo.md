### TsvTo tool help
	TsvTo (2025_12-49-gdb0c5d35)
	
	Converts TSV file to different table formats.
	
	Comment lines are not written to the output.
	
	Mandatory parameters:
	  -format <enum>    Output format.
	                    Valid: 'txt,md,html'
	
	Optional parameters:
	  -in <file>        Input TSV file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output file. If unset, writes to STDOUT.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TsvTo changelog
	TsvTo 2025_12-49-gdb0c5d35
	
	2025-12-11 Initial inplementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)