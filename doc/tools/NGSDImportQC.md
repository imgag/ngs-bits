### NGSDImportQC tool help
	NGSDImportQC (2024_08-113-g94a3b440)
	
	Imports QC terms into the NGSD.
	
	Mandatory parameters:
	  -obo <file>       HPO ontology file from 'https://raw.githubusercontent.com/imgag/ngs-bits/master/src/cppNGS/Resources/qcML.obo'.
	
	Optional parameters:
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	  -debug            Enable debug output.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDImportQC changelog
	NGSDImportQC 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)