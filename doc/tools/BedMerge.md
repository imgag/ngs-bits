### BedMerge tool help
	BedMerge (0.1-184-gc4d2f1b)
	
	Merges overlapping regions in a BED file.
	
	Optional parameters:
	  -in <file>    Input BED file. If unset, reads from STDIN.
	                Default value: ''
	  -out <file>   Output BED file. If unset, writes to STDOUT.
	                Default value: ''
	  -keep_b2b     Do not merge non-overlapping but adjacent (back-to-back) regions.
	                Default value: 'false'
	  -merge_names  Merge name columns instead of removing all annotations.
	                Default value: 'false'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --tdx         Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)