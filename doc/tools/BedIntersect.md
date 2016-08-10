### BedIntersect tool help
	BedIntersect (0.1-420-g3536bb0)
	
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
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedIntersect changelog
	BedIntersect 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)