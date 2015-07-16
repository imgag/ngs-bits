### VariantQC tool help
	VariantQC (0.1-46-gb124721)
	
	Calculates QC metrics on variant lists.
	
	Mandatory parameters:
	  -in <file>  Input VCF variant list. If a specific column
	
	Optional parameters:
	  -out <file> Output qcML file. If unset, writes to STDOUT.
	              Default value: ''
	  -txt        Writes TXT format instead of qcML.
	              Default value: 'false'
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)