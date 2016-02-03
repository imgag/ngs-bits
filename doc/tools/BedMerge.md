### BedMerge tool help
	BedMerge (0.1-190-g94e4c3d)
	
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
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)