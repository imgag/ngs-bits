### NGSDExportGff tool help
	NGSDExportGff (2024_08-113-g94a3b440)
	
	Writes all transcripts and exons of all genes to a gff3 file.
	
	Mandatory parameters:
	  -out <file>       The output GFF file.
	
	Optional parameters:
	  -genes            Add gene lines to group transcripts. This should be turned off when you want to use the file for IGV. This will also skip all transcripts which do not have a gene entry in NGSD.
	                    Default value: 'false'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDExportGff changelog
	NGSDExportGff 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)