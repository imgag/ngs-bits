### VariantAnnotateNGSD tool help
	VariantAnnotateNGSD (0.1-184-gc4d2f1b)
	
	Annotates a variant list with information from the NGSD.
	
	Mandatory parameters:
	  -in <file>       Input variant list.
	  -out <file>      Output variant list.
	
	Optional parameters:
	  -psname <string> Processed sample name. If set, this name is used instead of the file name to find the sample in the DB.
	                   Default value: ''
	  -ref <file>      Reference genome FASTA file for somatic mode. If unset 'reference_genome' from the 'settings.ini' file is used.
	                   Default value: ''
	  -mode <enum>     Determines annotation mode.
	                   Default value: 'germline'
	                   Valid: 'germline,somatic'
	  -test            Uses the test database instead of on the production database.
	                   Default value: 'false'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --tdx            Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)