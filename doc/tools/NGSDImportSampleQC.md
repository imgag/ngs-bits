### NGSDImportSampleQC tool help
	NGSDImportSampleQC (2024_08-110-g317f43b9)
	
	Imports QC metrics of a sample into NGSD.
	
	Mandatory parameters:
	  -ps <string>      Processed sample name.
	  -files <filelist> qcML files to import.
	
	Optional parameters:
	  -force            Overwrites already existing QC metrics instead of throwing an error.
	                    Default value: 'false'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDImportSampleQC changelog
	NGSDImportSampleQC 2024_08-110-g317f43b9
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)