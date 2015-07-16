### BedAnnotateGenes tool help
	BedAnnotateGenes (0.1-46-gb124721)
	
	Annotates the regions in a BED file with the name from a database BED file.
	
	Optional parameters:
	  -in <file>    Input BED file. If unset, reads from STDIN.
	                Default value: ''
	  -out <file>   Output BED file. If unset, writes to STDOUT.
	                Default value: ''
	  -db <file>    Database BED file containing names in the 4th column. If unset, 'ccds' from the 'settings.ini' file is used.
	                Default value: ''
	  -extend <int> The number of bases to extend the database regions at start/end before annotation.
	                Default value: '0'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --tdx         Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)