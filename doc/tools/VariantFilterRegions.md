### VariantFilterRegions tool help
	VariantFilterRegions (0.1-350-g2af2ea1)
	
	Filter a variant list based on a target region.
	
	Mandatory parameters:
	  -in <file>   Input variant list.
	  -out <file>  Output variant list. If unset, writes to STDOUT.
	
	Optional parameters:
	  -reg <file>  Input target region in BED format.
	               Default value: ''
	  -r <string>  Single target region in the format chr17:41194312-41279500.
	               Default value: ''
	  -invert      If used, the variants inside the target region are removed.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantFilterRegions changelog
	VariantFilterRegions 0.1-350-g2af2ea1
	
	2016-06-10 Added single target region parameter '-r'.
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)