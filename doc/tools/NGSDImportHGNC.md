### NGSDImportHGNC tool help
	NGSDImportHGNC (2025_03-80-g74f31dd7)
	
	Imports genes from the HGNC flat file.
	
	Mandatory parameters:
	  -in <file>        HGNC flat file (download https://storage.googleapis.com/public-download-files/hgnc/tsv/tsv/hgnc_complete_set.txt)
	  -ensembl <file>   Ensembl gene file (gff3) to resolve duplicate ENSG identifier (same as NGSDImportEnsembl 'in' parameter).
	
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
	
### NGSDImportHGNC changelog
	NGSDImportHGNC 2025_03-80-g74f31dd7
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)