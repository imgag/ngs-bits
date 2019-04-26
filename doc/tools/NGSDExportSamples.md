### NGSDExportSamples tool help
	NGSDExportSamples (2019_04-7-g0efa492)
	
	Lists processed samples from the NGSD.
	
	Optional parameters:
	  -out <file>           Output TSV file. If unset, writes to STDOUT.
	                        Default value: ''
	  -sample <string>      Sample name filter (substring match).
	                        Default value: ''
	  -species <string>     Species filter.
	                        Default value: ''
	  -no_bad_samples       If set, processed samples with 'bad' quality are excluded.
	                        Default value: 'false'
	  -no_tumor             If set, tumor samples are excluded.
	                        Default value: 'false'
	  -no_ffpe              If set, FFPE samples are excluded.
	                        Default value: 'false'
	  -project <string>     Project name filter.
	                        Default value: ''
	  -system <string>      Processing system name filter.
	                        Default value: ''
	  -run <string>         Sequencing run name filter.
	                        Default value: ''
	  -no_bad_runs          If set, sequencing runs with 'bad' quality are excluded.
	                        Default value: 'false'
	  -add_qc               If set, QC columns are added to output.
	                        Default value: 'false'
	  -add_outcome          If set, diagnostic outcome columns are added to output.
	                        Default value: 'false'
	  -add_disease_details  If set, disease details columns are added to output.
	                        Default value: 'false'
	  -add_path             Checks if the sample folder is present at the defaults location in the 'projects_folder' (as defined in the 'settings.ini' file).
	                        Default value: 'false'
	  -test                 Uses the test database instead of on the production database.
	                        Default value: 'false'
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDExportSamples changelog
	NGSDExportSamples 2019_04-7-g0efa492
	
	2019-04-12 Complete refactoring and interface change.
	2019-01-10 Added 'species' filter.
	2018-10-23 Added 'outcome' flag.
[back to ngs-bits](https://github.com/imgag/ngs-bits)