### NGSDImportOncotree tool help
	NGSDImportOncotree (2025_12-104-g15d04bf0)
	
	Imports Oncotree terms and their relations into the NGSD.
	
	Mandatory parameters:
	  -tree <file>      Oncotree JSON file from 'https://raw.githubusercontent.com/cBioPortal/oncotree/refs/heads/master/trees/oncotree_2025_10_03.json'.
	
	Optional parameters:
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	  -force            If set, overwrites old data.
	                    Default value: 'false'
	  -debug            Enables debug output
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDImportOncotree changelog
	NGSDImportOncotree 2025_12-104-g15d04bf0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)