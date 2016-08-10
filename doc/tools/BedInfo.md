### BedInfo tool help
	BedInfo (0.1-420-g3536bb0)
	
	Prints information about a (merged) BED file.
	
	Optional parameters:
	  -in <file>   Input BED file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output file. If unset, writes to STDOUT.
	               Default value: ''
	  -nomerge     If set, the input is not merged before printing statistics.
	               Default value: 'false'
	  -filename    If set, prints the input file name before each line.
	               Default value: 'false'
	  -fai <file>  If set, checks that the maximum position for each chromosome is not exceeded.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedInfo changelog
	BedInfo 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)