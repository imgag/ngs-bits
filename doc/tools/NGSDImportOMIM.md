### NGSDImportOMIM tool help
	NGSDImportOMIM (2024_08-113-g94a3b440)
	
	Imports OMIM genes/phenotypes into the NGSD.
	
	Note: This is an optional step since you might need and have a license for OMIM download.
	
	Mandatory parameters:
	  -gene <file>      OMIM 'mim2gene.txt' file from 'http://omim.org/downloads/'.
	  -morbid <file>    OMIM 'morbidmap.txt' file from 'http://omim.org/downloads/'.
	
	Optional parameters:
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	  -force            If set, overwrites old data.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDImportOMIM changelog
	NGSDImportOMIM 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)