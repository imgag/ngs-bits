### NGSDExportSamples tool help
	NGSDExportSamples (2021_03-16-ge75f73c2)
	
	Lists processed samples from the NGSD.
	
	Optional parameters:
	  -out <file>            Output TSV file. If unset, writes to STDOUT.
	                         Default value: ''
	  -sample <string>       Sample name filter (substring match).
	                         Default value: ''
	  -no_bad_samples        If set, processed samples with 'bad' quality are excluded.
	                         Default value: 'false'
	  -no_tumor              If set, tumor samples are excluded.
	                         Default value: 'false'
	  -no_ffpe               If set, FFPE samples are excluded.
	                         Default value: 'false'
	  -match_external_names  If set, also samples for which the external name matches 'sample' are exported.
	                         Default value: 'false'
	  -with_merged           If set, processed samples that were merged into another sample are included.
	                         Default value: 'false'
	  -species <string>      Species filter.
	                         Default value: ''
	  -sender <string>       Sample sender filter.
	                         Default value: ''
	  -study <string>        Processed sample study filter.
	                         Default value: ''
	  -project <string>      Project name filter.
	                         Default value: ''
	  -system <string>       Processing system name filter (short name).
	                         Default value: ''
	  -run <string>          Sequencing run name filter.
	                         Default value: ''
	  -run_finished          Only show samples where the analysis of the run is finished.
	                         Default value: 'false'
	  -run_device <string>   Sequencing run device name filter.
	                         Default value: ''
	  -no_bad_runs           If set, sequencing runs with 'bad' quality are excluded.
	                         Default value: 'false'
	  -add_qc                If set, QC columns are added to output.
	                         Default value: 'false'
	  -add_outcome           If set, diagnostic outcome columns are added to output.
	                         Default value: 'false'
	  -add_disease_details   If set, disease details columns are added to output.
	                         Default value: 'false'
	  -add_path <enum>       Adds a column with the given path type.
	                         Default value: ''
	                         Valid: ',SAMPLE_FOLDER,BAM,VCF,GSVAR,COPY_NUMBER_CALLS,STRUCTURAL_VARIANTS'
	  -add_report_config     Adds a column with report configuration information (exists/has_small_variants/has_cnvs).
	                         Default value: 'false'
	  -add_comments          Adds sample and processed sample comments columns.
	                         Default value: 'false'
	  -test                  Uses the test database instead of on the production database.
	                         Default value: 'false'
	
	Special parameters:
	  --help                 Shows this help and exits.
	  --version              Prints version and exits.
	  --changelog            Prints changeloge and exits.
	  --tdx                  Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDExportSamples changelog
	NGSDExportSamples 2021_03-16-ge75f73c2
	
	2021-04-13 Changed 'add_path' parameter to support different file/folder types.
	2020-10-08 Added parameters 'sender' and 'study'.
	2020-07-20 Added 'match_external_names' flag.
	2019-12-11 Added 'run_finished' and 'add_report_config' flags.
	2019-05-17 Added 'with_merged' flag.
	2019-04-12 Complete refactoring and interface change.
	2019-01-10 Added 'species' filter.
	2018-10-23 Added 'outcome' flag.
[back to ngs-bits](https://github.com/imgag/ngs-bits)