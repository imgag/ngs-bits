### NGSDImportUCSC tool help
	NGSDImportUCSC (0.1-184-gc4d2f1b)
	
	Imports transcript information into NGSD (download from http://hgdownload.cse.ucsc.edu/goldenPath/hg19/database/).
	
	Mandatory parameters:
	  -ccds <file>   UCSC ccdsGene.txt file
	  -ccdsKM <file> UCSC ccdsKgMap.txt file
	  -kg <file>     UCSC knownGene.txt file
	  -kgXR <file>   UCSC kgXref.txt file
	
	Optional parameters:
	  -test          Uses the test database instead of on the production database.
	                 Default value: 'false'
	  -force         If set, overwrites old data.
	                 Default value: 'false'
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --tdx          Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)