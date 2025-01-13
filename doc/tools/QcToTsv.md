### QcToTsv tool help
	QcToTsv (2024_11-59-ge0a7288e)
	
	Converts qcML files to a TSV file..
	
	Mandatory parameters:
	  -in <filelist>    Input qcML files.
	
	Optional parameters:
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -obo <file>       OBO file to use. If unset, uses the default file compiled into ngs-bits.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### QcToTsv changelog
	QcToTsv 2024_11-59-ge0a7288e
	
	2025-01-10 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)