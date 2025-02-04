### NGSDAnnotateSV tool help
	NGSDAnnotateSV (2024_11-41-g79ac725e)
	
	Annotates the structural variants in a given BEDPE file with the count of pathogenic SVs of classes 4 and 5 found in the NGSD.
	
	Mandatory parameters:
	  -in <file>        BEDPE file containing structural variants.
	  -out <file>       Output BEDPE file containing annotated structural variants.
	
	Optional parameters:
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDAnnotateSV changelog
	NGSDAnnotateSV 2024_11-41-g79ac725e
	
	2024-12-17 Refactored to only annotate pathogenic SVs from NGSD
	2020-03-12 Bugfix in match computation for INS and BND
	2020-03-11 Updated match computation for INS and BND
	2020-02-27 Added temporary db table with same processing system.
	2020-02-21 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)