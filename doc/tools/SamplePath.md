### SamplePath tool help
	SamplePath (2024_08-113-g94a3b440)
	
	Prints the folder of a processed sample.
	
	Mandatory parameters:
	  -ps <string>      Processed sample name.
	
	Optional parameters:
	  -type <enum>      Path type to print.
	                    Default value: 'SAMPLE_FOLDER'
	                    Valid: 'SAMPLE_FOLDER,BAM,VCF,GSVAR,COPY_NUMBER_CALLS,STRUCTURAL_VARIANTS'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### SamplePath changelog
	SamplePath 2024_08-113-g94a3b440
	
	2023-07-18 Initial version
[back to ngs-bits](https://github.com/imgag/ngs-bits)