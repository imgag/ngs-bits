### TranscriptComparison tool help
	TranscriptComparison (2025_07-127-g60fc6b39)
	
	Compares transcripts from Ensembl and RefSeq/CCDS.
	
	Mandatory parameters:
	  -ensembl <file>   Ensembl GFF file.
	  -refseq <file>    RefSeq GFF file.
	
	Optional parameters:
	  -out <file>       Output TSV file with matches.
	                    Default value: ''
	  -min_ol <float>   Minimum overall/CDS overlap percentage for printing out a relation if there is no perfect match (disabled by default).
	                    Default value: '100'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TranscriptComparison changelog
	TranscriptComparison 2025_07-127-g60fc6b39
	
	2024-09-26 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)