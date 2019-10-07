### NGSDImportORPHA tool help
	NGSDImportORPHA (2019_09-10-g5f65b37)
	
	Imports ORPHA diseases/genes into the NGSD.
	
	Mandatory parameters:
	  -terms <file> Terms XML file from 'https://github.com/Orphanet/Orphadata.org/tree/master/Disorders%20cross%20referenced%20with%20other%20nomenclatures'.
	  -genes <file> Terms<>genes XML file from 'https://github.com/Orphanet/Orphadata.org/tree/master/Disorders%20with%20their%20associated%20genes'.
	
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
	NGSDImportORPHA 2019_09-10-g5f65b37
	
	2019-09-30 Initial version
[back to ngs-bits](https://github.com/imgag/ngs-bits)