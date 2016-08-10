### NGSDImportHGNC tool help
	NGSDImportHGNC (0.1-420-g3536bb0)
	
	Imports genes from the HGNC flat file.
	
	Mandatory parameters:
	  -in <file>   HGNC flat file (download and unzip ftp://ftp.ebi.ac.uk/pub/databases/genenames/hgnc_complete_set.txt.gz)
	
	Optional parameters:
	  -test        Uses the test database instead of on the production database.
	               Default value: 'false'
	  -force       If set, overwrites old data.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportHGNC changelog
	NGSDImportHGNC 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)