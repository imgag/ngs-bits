### GenesToTranscripts tool help
	GenesToTranscripts (2024_08-113-g94a3b440)
	
	Converts a text file with gene names to a TSV file with two columns (transcript, gene name).
	
	Best transcript is determined according this order: 'preferred transcript in NGSD', 'MANE select', 'Ensembl canonical', 'longest coding transcript', 'longest transcript'
	Relevant transcripts are: 'preferred transcript in NGSD', 'MANE select', 'MANE plus clinical', 'Ensembl canonical' (if none of those exist, the longest coding or longest transcript is used)
	
	Mandatory parameters:
	  -mode <enum>      Mode: all = all transcripts, best = best transcript, relevant = all relevant transcripts.
	                    Valid: 'all,best,relevant'
	
	Optional parameters:
	  -in <file>        Input TXT file with one gene symbol per line. If unset, reads from STDIN.
	                    Default value: ''
	  -version          Append transcript version to transcript name.
	                    Default value: 'false'
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
	
### GenesToTranscripts changelog
	GenesToTranscripts 2024_08-113-g94a3b440
	
	2023-05-26 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)