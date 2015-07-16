### BedSubtract tool help
	BedSubtract (0.1-46-gb124721)
	
	Subtracts the regions in one BED file from another.
	
	Mandatory parameters:
	  -in2 <file> Input BED file which is subtracted from 'in'.
	
	Optional parameters:
	  -in <file>  Input BED file. If unset, reads from STDIN.
	              Default value: ''
	  -out <file> Output BED file. If unset, writes to STDOUT.
	              Default value: ''
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)