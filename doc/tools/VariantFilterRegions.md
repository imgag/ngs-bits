### VariantFilterRegions tool help
	VariantFilterRegions (0.1-184-gc4d2f1b)
	
	Filters variant lists accordning to a target region.
	
	Mandatory parameters:
	  -in <file>  Input variant list.
	  -reg <file> Input target region in BED format.
	  -out <file> Output variant list. If unset, writes to STDOUT.
	
	Optional parameters:
	  -invert     If used, the variants inside the target region are removed.
	              Default value: 'false'
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)