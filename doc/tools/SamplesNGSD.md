### SamplesNGSD tool help
	SamplesNGSD (0.1-184-gc4d2f1b)
	
	Lists processed samples from NGSD.
	
	Optional parameters:
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -project <string> Project name filter.
	                    Default value: ''
	  -sys <string>     Processing system short name filter.
	                    Default value: ''
	  -run <string>     Sequencing run name filter.
	                    Default value: ''
	  -quality <enum>   Minimum processed sample/sample/run quality filter.
	                    Default value: 'bad'
	                    Valid: 'bad,medium,good'
	  -no_tumor         If set, tumor samples are excluded.
	                    Default value: 'false'
	  -no_ffpe          If set, FFPE samples are excluded.
	                    Default value: 'false'
	  -qc               If set, QC values are appended.
	                    Default value: 'false'
	  -check_path       Checks if the sample folder is present at the defaults location in the 'projects_folder' (as defined in the 'settings.ini' file).
	                    Default value: 'false'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --tdx             Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)