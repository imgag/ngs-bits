### GenesToBed tool help
	GenesToBed (2024_08-113-g94a3b440)
	
	Converts a text file with gene names to a BED file.
	
	Mandatory parameters:
	  -source <enum>    Transcript source database.
	                    Valid: 'ccds,ensembl'
	  -mode <enum>      Mode: gene = start/end of all transcripts, exon = start/end of all exons of all transcripts.
	                    Valid: 'gene,exon'
	
	Optional parameters:
	  -in <file>        Input TXT file with one gene symbol per line. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output BED file. If unset, writes to STDOUT.
	                    Default value: ''
	  -fallback         Allow fallback to all source databases, if no transcript for a gene is defined in the selected source database.
	                    Default value: 'false'
	  -anno             Annotate transcript identifier in addition to gene name.
	                    Default value: 'false'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### GenesToBed changelog
	GenesToBed 2024_08-113-g94a3b440
	
	2017-02-09 Added option to annotate transcript names.
[back to ngs-bits](https://github.com/imgag/ngs-bits)