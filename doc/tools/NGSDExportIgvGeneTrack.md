### NGSDExportIgvGeneTrack tool help
	NGSDExportIgvGeneTrack (2024_08-113-g94a3b440)
	
	Writes all transcripts and exons of all genes to a IGV-readable text file.
	
	Mandatory parameters:
	  -out <file>       The output text file.
	
	Optional parameters:
	  -out_mane <file>  The optional output text file containing only MANE transcripts.
	                    Default value: ''
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDExportIgvGeneTrack changelog
	NGSDExportIgvGeneTrack 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)