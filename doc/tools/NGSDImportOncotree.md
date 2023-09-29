### NGSDImportOncotree tool help
	NGSDImportOncotree (2023_06-98-g044e3ed3)
	
	Imports Oncotree terms and their relations into the NGSD.
	
	Mandatory parameters:
	  -tree <file> Oncotree JSON file from 'https://oncotree.mskcc.org/api/tumorTypes/tree'.
	
	Optional parameters:
	  -test        Uses the test database instead of on the production database.
	               Default value: 'false'
	  -force       If set, overwrites old data.
	               Default value: 'false'
	  -debug       Enables debug output
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportOncotree changelog
	NGSDImportOncotree 2023_06-98-g044e3ed3
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)