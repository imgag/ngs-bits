### VcfSort tool help
	VcfSort (2020_03-260-ge35d12de)
	
	Sorts variant lists according to chromosomal position.
	
	Mandatory parameters:
	  -in <file>   Input variant list in VCF format.
	  -out <file>  Output variant list.
	
	Optional parameters:
	  -qual        Also sort according to variant quality. Ignored if 'fai' file is given.
	               Default value: 'false'
	  -fai <file>  FAI file defining different chromosome order.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfSort changelog
	VcfSort 2020_03-260-ge35d12de
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)