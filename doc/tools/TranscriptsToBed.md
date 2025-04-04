### TranscriptsToBed tool help
	TranscriptsToBed (2024_08-113-g94a3b440)
	
	Converts a text file with transcript names to a BED file.
	
	Mandatory parameters:
	  -mode <enum>      Mode: gene = start/end of the transcript, exon = start/end of all exons of the transcript.
	                    Valid: 'gene,exon'
	
	Optional parameters:
	  -in <file>        Input TXT file with one transcript name per line. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output BED file. If unset, writes to STDOUT.
	                    Default value: ''
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TranscriptsToBed changelog
	TranscriptsToBed 2024_08-113-g94a3b440
	
	2023-05-25 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)