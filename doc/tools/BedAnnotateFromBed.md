### BedAnnotateFromBed tool help
	BedAnnotateFromBed (0.1-893-g1246b60)
	
	Annotates BED file regions with information from a second BED file.
	
	Mandatory parameters:
	  -in2 <file>  BED file that is used as annotation source (4th column is used as annotation value, if not presend 'yes' is used).
	
	Optional parameters:
	  -in <file>   Input BED file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output BED file. If unset, writes to STDOUT.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedAnnotateFromBed changelog
	BedAnnotateFromBed 0.1-893-g1246b60
	
	2017-11-03 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)