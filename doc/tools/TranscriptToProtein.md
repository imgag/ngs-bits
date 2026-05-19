### TranscriptToProtein tool help
	TranscriptToProtein (2025_12-266-g396e1fe11)
	
	Computes the protein sequence for each transcript name given.
	
	Mandatory parameters:
	  -out <file>       Output TSV file.
	
	Optional parameters:
	  -in <file>        Input file. If unset, reads from STDIN. Expects one transcript ID (ENSEMBLE or REFSEQ) per line.
	                    Default value: ''
	  -ref <file>       Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                    Default value: ''
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	  -build <enum>     Genome build
	                    Default value: 'hg38'
	                    Valid: 'hg19,hg38'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TranscriptToProtein changelog
	TranscriptToProtein 2025_12-266-g396e1fe11
	
	2026-05-04 Initial version
[back to ngs-bits](https://github.com/imgag/ngs-bits)