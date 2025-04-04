### VariantRanking tool help
	VariantRanking (2024_08-110-g317f43b9)
	
	Annotatates variants in GSvar format with a score/rank.
	
	Mandatory parameters:
	  -in <file>                  Input variant list in GSvar format.
	  -hpo_ids <string>           Comma-separated list of HPO identifiers.
	  -out <file>                 Output variant list in GSvar format.
	
	Optional parameters:
	  -algorithm <enum>           Algorithm used for ranking.
	                              Default value: 'GSvar_v1'
	                              Valid: 'GSvar_v1,GSvar_v2_dominant,GSvar_v2_recessive'
	  -add_explanation            Add a third column with an explanation how that score was calculated.
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
	VariantRanking 2024_08-110-g317f43b9
	
	2020-11-20 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)