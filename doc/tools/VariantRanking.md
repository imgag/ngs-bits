### VariantRanking tool help
	VariantRanking (2025_07-53-gefb5888f)
	
	Rankes small variants in the context of a patients phenotype using an evidence-based model.
	
	Mandatory parameters:
	  -in <file>                  Input variant list in GSvar format.
	  -hpo_ids <string>           Comma-separated list of HPO identifiers.
	  -out <file>                 Output variant list in GSvar format with rank/score columns.
	  -algorithm <enum>           Algorithm used for ranking.
	                              Valid: 'GSvar_v1,GSvar_v2_dominant,GSvar_v2_recessive'
	
	Optional parameters:
	  -add_explanation            Add a third output column with an explanation how that score was calculated.
	                              Default value: 'false'
	  -use_blacklist              Use variant blacklist from settings.ini file.
	                              Default value: 'false'
	  -skip_ngsd_classifications  Do not use variant classifications from NGSD.
	                              Default value: 'false'
	  -test                       Uses the test database instead of on the production database.
	                              Default value: 'false'
	
	Special parameters:
	  --help                      Shows this help and exits.
	  --version                   Prints version and exits.
	  --changelog                 Prints changeloge and exits.
	  --tdx                       Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]           Settings override file (no other settings files are used).
	
### VariantRanking changelog
	VariantRanking 2025_07-53-gefb5888f
	
	2023-05-05 Implementation of v2.
	2020-11-20 Implementation of v1.
[back to ngs-bits](https://github.com/imgag/ngs-bits)