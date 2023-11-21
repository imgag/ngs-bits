### NGSDSameSample tool help
	NGSDSameSample (2023_09-93-gad5c47c9)
	
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
	  -test                 Uses the test database instead of on the production database.
	                        Default value: 'false'
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDSameSample changelog
	NGSDSameSample 2023_09-93-gad5c47c9
	
	2023-11-15 initial commit
[back to ngs-bits](https://github.com/imgag/ngs-bits)