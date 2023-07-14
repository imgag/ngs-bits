### NGSDImportSampleQC tool help
	NGSDImportSampleQC (2023_03-107-g2a1d2478)
	
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
	
### NGSDImportSampleQC changelog
	NGSDImportSampleQC 2023_03-107-g2a1d2478
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)