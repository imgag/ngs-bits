### VariantAnnotateNGSD tool help
	VariantAnnotateNGSD (0.1-420-g3536bb0)
	
	Annotates a variant list with information from the NGSD.
	
	Mandatory parameters:
	  -in <file>       Input variant list.
	  -out <file>      Output variant list.
	
	Optional parameters:
	  -psname <string> Processed sample name. If set, this name is used instead of the file name to find the sample in the DB.
	                   Default value: ''
	  -mode <enum>     Determines annotation mode.
	                   Default value: 'germline'
	                   Valid: 'germline,somatic'
	  -test            Uses the test database instead of on the production database.
	                   Default value: 'false'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --changelog      Prints changeloge and exits.
	  --tdx            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantAnnotateNGSD changelog
	VariantAnnotateNGSD 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)