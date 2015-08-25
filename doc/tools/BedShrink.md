### BedShrink tool help
	BedShrink (0.1-52-g9f9161f)
	
	Shrinks the regions in a BED file.
	
	Mandatory parameters:
	  -n <int>    The number of bases to shrink (on both sides of each region).
	
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