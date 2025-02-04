### BedGeneOverlap tool help
	BedGeneOverlap (2024_08-113-g94a3b440)
	
	Calculates how much of each overlapping gene is covered.
	
	Mandatory parameters:
	  -source <enum>    Transcript source database.
	                    Valid: 'ccds,ensembl'
	
	Optional parameters:
	  -in <file>        Input BED file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedGeneOverlap changelog
	BedGeneOverlap 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)