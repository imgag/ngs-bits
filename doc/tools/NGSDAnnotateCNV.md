### NGSDAnnotateCNV tool help
	NGSDAnnotateCNV (2024_11-59-ge0a7288e)
	
	Annotates a CNV file with overlaping pathogenic CNVs from NGSD.
	
	Mandatory parameters:
	  -in <file>        TSV file containing CNV.
	  -out <file>       TSV output file.
	
	Optional parameters:
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDAnnotateCNV changelog
	NGSDAnnotateCNV 2024_11-59-ge0a7288e
	
	2024-11-28 Imporved annotation of overlapping pathogenic CNVs.
	2020-02-21 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)