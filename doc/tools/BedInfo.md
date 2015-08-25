### BedInfo tool help
	BedInfo (0.1-52-g9f9161f)
	
	Prints information about a (merged) BED file.
	
	Optional parameters:
	  -in <file>  Input BED file. If unset, reads from STDIN.
	              Default value: ''
	  -out <file> Output file. If unset, writes to STDOUT.
	              Default value: ''
	  -nomerge    If set, the input is not merged before printing statistics.
	              Default value: 'false'
	  -filename   If set, prints the input file name before each line.
	              Default value: 'false'
	  -fai <file> If set, checks that the maximum position for each chromosome is not exceeded.
	              Default value: ''
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)