### NGSDImportGenlab tool help
	NGSDImportGenlab (2023_06-98-g044e3ed3)
	
	Import sample information from GenLAB into NGSD.
	
	Imports the following data:
	general meta data: gender, patient ID, year of birth, disease data, studies
	sample relations:
	rna tissue:
	
	Mandatory parameters:
	  -ps <string>    Processed sample for which the GenLAB data will be imported.
	
	Optional parameters:
	  -no_relations   Do not search and import sample relations from GenLAB.
	                  Default value: 'false'
	  -no_rna_tissue  Do not import RNA reference tissue from HPO terms.
	                  Default value: 'false'
	  -no_metadata    Do not search and import metadata from GenLAB (disease group, ICD10, HPO, ...)
	                  Default value: 'false'
	  -test           Uses the test database instead of on the production database.
	                  Default value: 'false'
	  -dry_run        Run as specified but do NOT change anything in the database.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportGenlab changelog
	NGSDImportGenlab 2023_06-98-g044e3ed3
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)