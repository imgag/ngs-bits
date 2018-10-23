### NGSDExportSamples tool help
	NGSDExportSamples (2018_06-59-g24102d3)
	
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
	  -qc               If set, QC colums are added to output.
	                    Default value: 'false'
	  -outcome          If set, diagnostic outcome colums are added to output.
	                    Default value: 'false'
	  -check_path       Checks if the sample folder is present at the defaults location in the 'projects_folder' (as defined in the 'settings.ini' file).
	                    Default value: 'false'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDExportSamples changelog
	NGSDExportSamples 2018_06-59-g24102d3
	
	2018-10-23 Added 'outcome' flag.
[back to ngs-bits](https://github.com/imgag/ngs-bits)