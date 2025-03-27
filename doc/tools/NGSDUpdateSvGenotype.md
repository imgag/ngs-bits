### NGSDUpdateSvGenotype tool help
	NGSDUpdateSvGenotype (2024_08-114-gef8b0523)
	
	Updates the genotype for a given processed sample.
	
	Mandatory parameters:
	  -ps <string>      Processed sample id.
	  -in <file>        Input BEDPE file.
	
	Optional parameters:
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDUpdateSvGenotype changelog
	NGSDUpdateSvGenotype 2024_08-114-gef8b0523
	
	2022-03-07 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)