### BedSort tool help
	BedSort (0.1-46-gb124721)
	
	Sort the regions in a BED file.
	
	Optional parameters:
	  -in <file>  Input BED file. If unset, reads from STDIN.
	              Default value: ''
	  -out <file> Output BED file. If unset, writes to STDOUT.
	              Default value: ''
	  -uniq       If set, subsequent duplicate entries are removed after sorting.
	              Default value: 'false'
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)