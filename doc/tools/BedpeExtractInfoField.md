### BedpeExtractInfoField tool help
	BedpeExtractInfoField (2023_11-133-g87eceb58)
	
	Extract a given INFO field key and annotates it as a separate column.
	
	Mandatory parameters:
	  -info_fields <string> Comma separate list of INFO keys (and column header names) which should be extracted: "INFO_KEY1[:COLUMN_HEADER1],INFO_KEY2[:COLUMN_HEADER2],..."
	
	Optional parameters:
	  -in <file>            Input BEDPE file. If unset, reads from STDIN.
	                        Default value: ''
	  -out <file>           Output BEDPE file. If unset, writes to STDOUT.
	                        Default value: ''
	  -info_column <string> Header name of the INFO column.
	                        Default value: 'INFO_A'
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedpeExtractInfoField changelog
	BedpeExtractInfoField 2023_11-133-g87eceb58
	
	2024-01-18 Removed single sample restriction
	2023-10-04 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)