### NGSDAnnotateSV tool help
	NGSDAnnotateSV (2024_08-110-g317f43b9)
	
	Annotates the structural variants of a given BEDPE file by the NGSD counts.
	
	NOTICE: the parameter '-ignore_processing_system' will also use SVs from low quality samples (bad samples).
	
	Mandatory parameters:
	  -in <file>                 BEDPE file containing structural variants.
	  -out <file>                Output BEDPE file containing annotated structural variants.
	  -ps <string>               Processed sample name.
	
	Optional parameters:
	  -test                      Uses the test database instead of on the production database.
	                             Default value: 'false'
	  -ignore_processing_system  Use all SVs for annotation (otherwise only SVs from good samples of the same processing system are used)
	                             Default value: 'false'
	  -debug                     Provide additional information in STDOUT (e.g. query runtime)
	                             Default value: 'false'
	  -use_memory                Creates the temporary tables in memory.
	                             Default value: 'false'
	
	Special parameters:
	  --help                     Shows this help and exits.
	  --version                  Prints version and exits.
	  --changelog                Prints changeloge and exits.
	  --tdx                      Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]          Settings override file (no other settings files are used).
	
### NGSDAnnotateSV changelog
	NGSDAnnotateSV 2024_08-110-g317f43b9
	
	2020-03-12 Bugfix in match computation for INS and BND
	2020-03-11 Updated match computation for INS and BND
	2020-02-27 Added temporary db table with same processing system.
	2020-02-21 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)