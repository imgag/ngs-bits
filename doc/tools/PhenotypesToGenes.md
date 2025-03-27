### PhenotypesToGenes tool help
	PhenotypesToGenes (2024_08-110-g317f43b9)
	
	Converts a phenotype list to a list of matching genes.
	
	For each given HPO term, the genes associated with the term itself and the genes associated with any sub-term are returned.
	
	Optional parameters:
	  -in <string>           Input file, containing one HPO term identifier per line, e.g. HP:0002066. Text after the identifier is ignored. If unset, reads from STDIN.
	                         Default value: ''
	  -out <file>            Output TSV file with genes (column 1) and matched phenotypes (column 2). If unset, writes to STDOUT.
	                         Default value: ''
	  -test                  Uses the test database instead of on the production database.
	                         Default value: 'false'
	  -ignore_invalid        Ignores invalid HPO identifiers instead of throwing an error.
	                         Default value: 'false'
	  -ignore_non_phenotype  Ignores HPO identifiers that are sub-terms of 'Mode of inheritance' or 'Frequency'
	                         Default value: 'false'
	  -source <string>       Comma-separated list of phenotype-gene source databases.
	                         Default value: 'HPO,OMIM,ClinVar,Decipher,HGMD,GenCC'
	  -evidence <string>     Comma-separated list of phenotype-gene evidence levels.
	                         Default value: 'n/a,low,medium,high'
	
	Special parameters:
	  --help                 Shows this help and exits.
	  --version              Prints version and exits.
	  --changelog            Prints changeloge and exits.
	  --tdx                  Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]      Settings override file (no other settings files are used).
	
### PhenotypesToGenes changelog
	PhenotypesToGenes 2024_08-110-g317f43b9
	
	2020-11-23 Added parameter 'ignore_invalid'.
	2020-05-24 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)