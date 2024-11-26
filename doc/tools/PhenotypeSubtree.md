### PhenotypeSubtree tool help
	PhenotypeSubtree (2024_08-113-g94a3b440)
	
	Returns all sub-phenotype of a given phenotype.
	
	Mandatory parameters:
	  -in <string>      HPO phenotype identifier, e.g. HP:0002066.
	
	Optional parameters:
	  -out <file>       Output TSV file with phenotypes identifiers (column 1) and names (column 2). If unset, writes to STDOUT.
	                    Default value: ''
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### PhenotypeSubtree changelog
	PhenotypeSubtree 2024_08-113-g94a3b440
	
	2020-05-26 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)