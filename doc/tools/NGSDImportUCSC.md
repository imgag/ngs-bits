### NGSDImportUCSC tool help
	NGSDImportUCSC (0.1-190-g94e4c3d)
	
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
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)