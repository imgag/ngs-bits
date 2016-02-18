### VcfSort tool help
	VcfSort (0.1-190-g94e4c3d)
	
	Sorts variant lists according to chromosomal position.
	
	Mandatory parameters:
	  -in <file>  Input variant list.
	  -out <file> Output variant list.
	
	Optional parameters:
	  -qual       Also sort according to variant quality. Ignored if 'fai' file is given.
	              Default value: 'false'
	  -fai <file> FAI file defining different chromosome order.
	              Default value: ''
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)