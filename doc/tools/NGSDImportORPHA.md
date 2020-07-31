### NGSDImportORPHA tool help
	NGSDImportORPHA (2020_06-40-g7bb4fdf2)
	
	Imports ORPHA diseases/genes into the NGSD.
	
	Mandatory parameters:
	  -terms <file> Terms XML file from 'http://www.orphadata.org/data/xml/en_product1.xml'.
	  -genes <file> Terms<>genes XML file from 'http://www.orphadata.org/data/xml/en_product6.xml'.
	
	Optional parameters:
	  -test         Uses the test database instead of on the production database.
	                Default value: 'false'
	  -force        If set, overwrites old data.
	                Default value: 'false'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --changelog   Prints changeloge and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportORPHA changelog
	NGSDImportORPHA 2020_06-40-g7bb4fdf2
	
	2019-09-30 Initial version
[back to ngs-bits](https://github.com/imgag/ngs-bits)