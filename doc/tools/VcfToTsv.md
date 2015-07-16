### VcfToTsv tool help
	VcfToTsv (0.1-46-gb124721)
	
	Shifts indels in a variant list as far to the left as possible. Complex indels and multi-allelic deletions are not shifted!
	
	Mandatory parameters:
	  -in <file>  Input variant list in VCF format.
	  -out <file> Output variant list in TSV format.
	
	Optional parameters:
	  -split      Split multi-allelic variants.
	              Default value: 'false'
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)