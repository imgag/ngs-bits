### NGSDExportSamples tool help
	NGSDExportSamples (2024_07-35-gb854a4ba)
	
	Lists processed samples from the NGSD.
	
	Optional parameters:
	  -out <file>                Output TSV file. If unset, writes to STDOUT.
	                             Default value: ''
	  -sample <string>           Sample name filter (substring match).
	                             Default value: ''
	  -no_bad_samples            If set, processed samples with 'bad' quality are excluded.
	                             Default value: 'false'
	  -no_tumor                  If set, tumor samples are excluded.
	                             Default value: 'false'
	  -no_normal                 If set, germline samples are excluded.
	                             Default value: 'false'
	  -no_ffpe                   If set, FFPE samples are excluded.
	                             Default value: 'false'
	  -match_external_names      If set, also samples for which the external name matches 'sample' are exported.
	                             Default value: 'false'
	  -with_merged               If set, processed samples that were merged into another sample are included.
	                             Default value: 'false'
	  -only_with_small_variants  If set, only processed samples that have small variants in NGSD are listed.
	                             Default value: 'false'
	  -species <string>          Species filter.
	                             Default value: ''
	  -tissue <string>           Tissue filter.
	                             Default value: ''
	  -ancestry <string>         Ancestry filter.
	                             Default value: ''
	  -disease_group <string>    Disease group filter
	                             Default value: ''
	  -disease_status <string>   Disease status filter
	                             Default value: ''
	  -phenotypes <string>       HPO phenotype identifiers separated by colon, e.g. 'HP:0002066;HP:0004322'
	                             Default value: ''
	  -sender <string>           Sample sender filter.
	                             Default value: ''
	  -study <string>            Processed sample study filter.
	                             Default value: ''
	  -project <string>          Project name filter.
	                             Default value: ''
	  -project_type <string>     Project type filter
	                             Default value: ''
	  -no_archived_projects      If set, samples in archived projects are excluded.
	                             Default value: 'false'
	  -system <string>           Processing system name filter (short name).
	                             Default value: ''
	  -system_type <string>      Type of processing system filter
	                             Default value: ''
	  -run <string>              Sequencing run name filter.
	                             Default value: ''
	  -run_finished              Only show samples where the analysis of the run is finished.
	                             Default value: 'false'
	  -run_device <string>       Sequencing run device name filter.
	                             Default value: ''
	  -run_before <string>       Sequencing run before or equal to the given date.
	                             Default value: ''
	  -run_after <string>        Sequencing run after or equal to the given date.
	                             Default value: ''
	  -no_bad_runs               If set, sequencing runs with 'bad' quality are excluded.
	                             Default value: 'false'
	  -add_qc                    If set, QC columns are added to output.
	                             Default value: 'false'
	  -add_outcome               If set, diagnostic outcome columns are added to output.
	                             Default value: 'false'
	  -add_disease_details       If set, disease details columns are added to the output.
	                             Default value: 'false'
	  -add_path <enum>           Adds a column with the given path type.
	                             Default value: ''
	                             Valid: ',SAMPLE_FOLDER,BAM,VCF,GSVAR,COPY_NUMBER_CALLS,STRUCTURAL_VARIANTS'
	  -add_report_config         Adds a column with report configuration information (if it exists and if causal variants exist).
	                             Default value: 'false'
	  -add_comments              Adds sample and processed sample comments columns.
	                             Default value: 'false'
	  -add_normal_sample         Adds a column with the normal germline sample associated to a tumor samples.
	                             Default value: 'false'
	  -add_dates                 Adds four columns with year of birth, order date, sampling date and sample receipt date.
	                             Default value: 'false'
	  -add_call_details          Adds variant caller and version and variant calling date columns for small variants, CNVs and SVs.
	                             Default value: 'false'
	  -add_lab_columns           Adds columns input, molarity, operator, processing method and batch number.
	                             Default value: 'false'
	  -add_study_column          Add a column with studies of the sample.
	                             Default value: 'false'
	  -test                      Uses the test database instead of on the production database.
	                             Default value: 'false'
	
	Special parameters:
	  --help                     Shows this help and exits.
	  --version                  Prints version and exits.
	  --changelog                Prints changeloge and exits.
	  --tdx                      Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDExportSamples changelog
	NGSDExportSamples 2024_07-35-gb854a4ba
	
	2024-08-21 Added 'add_study_column' flag.
	2024-04-24 Added 'only_with_small_variants' flag.
	2024-03-04 Added 'add_lab_columns' flag.
	2023-11-16 Added 'add_call_details' flag.
	2023-07-13 Added 'add_dates' flag.
	2022-11-11 Added 'ancestry' and 'phenotypes' filter options.
	2022-03-03 Added 'disease_group', 'disease_status', 'project_type' and 'tissue' filter options.
	2021-04-29 Added 'run_before' filter option.
	2021-04-16 Added ancestry column.
	2021-04-13 Changed 'add_path' parameter to support different file/folder types.
	2020-10-08 Added parameters 'sender' and 'study'.
	2020-07-20 Added 'match_external_names' flag.
	2019-12-11 Added 'run_finished' and 'add_report_config' flags.
	2019-05-17 Added 'with_merged' flag.
	2019-04-12 Complete refactoring and interface change.
	2019-01-10 Added 'species' filter.
	2018-10-23 Added 'outcome' flag.
[back to ngs-bits](https://github.com/imgag/ngs-bits)