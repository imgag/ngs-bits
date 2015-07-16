### BedIntersect tool help
	BedIntersect (0.1-46-gb124721)
	
	Intersects the regions in two BED files.
	
	Mandatory parameters:
	  -in2 <file>  Second input BED file.
	
	Optional parameters:
	  -mode <enum> Output mode: intersect of both files (intersect), original entry of file 1 (in) or original entry of file 2 (in2).
	               Default value: 'intersect'
	               Valid: 'intersect,in,in2'
	  -in <file>   Input BED file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output BED file. If unset, writes to STDOUT.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --tdx        Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)