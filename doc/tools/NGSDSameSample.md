### NGSDSameSample tool help
	NGSDSameSample (2024_08-110-g317f43b9)
	
	For the given processed sample, lists all processed samples of the same patient or sample.
	
	Does not contain the given processed sample itself.
	
	Mandatory parameters:
	  -ps <string>          Processed sample name.
	
	Optional parameters:
	  -out <file>           Output TSV file. If unset, writes to STDOUT.
	                        Default value: ''
	  -sample_type <string> Comma-separated list of sample types.
	                        Default value: ''
	  -system_type <string> Comma-separated list of processing system types.
	                        Default value: ''
	  -system <string>      Comma-separated list of processing system (short) names.
	                        Default value: ''
	  -mode <enum>          Type of relation (either only same-sample or same-patient (includes same-sample).
	                        Default value: 'SAME_PATIENT'
	                        Valid: 'SAME_SAMPLE,SAME_PATIENT'
	  -include_bad          Include bad processed samples in the output (will be ignored on default).
	                        Default value: 'false'
	  -include_merged       Include merged quality processed samples in the output (will be ignored on default).
	                        Default value: 'false'
	  -test                 Uses the test database instead of on the production database.
	                        Default value: 'false'
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]     Settings override file (no other settings files are used).
	
### NGSDSameSample changelog
	NGSDSameSample 2024_08-110-g317f43b9
	
	2024-08-22 remove bad samples by default
	2023-11-21 remove merged samples by default
	2023-11-15 initial commit
[back to ngs-bits](https://github.com/imgag/ngs-bits)