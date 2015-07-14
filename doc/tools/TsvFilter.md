### TsvFilter tool help
	TsvFilter (0.1-33-gfdb89ad)
	
	Filters the rows of a TSV file according to the value of a specific column.
	
	Mandatory parameters:
	  -filter <string> Filter string with column name, operation and value,e.g. 'depth > 17'.
	Valid operations are '>','>=','=','<=','<','is','contains'.
	
	Optional parameters:
	  -in <file>       Input TSV file. If unset, reads from STDIN.
	                   Default value: ''
	  -out <file>      Output file. If unset, writes to STDOUT.
	                   Default value: ''
	  -numeric         If set, column name is interpreted as a 1-based column number.
	                   Default value: 'false'
	  -v               Invert filter.
	                   Default value: 'false'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --tdx            Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits]("https://github.com/marc-sturm/ngs-bits")