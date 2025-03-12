### BedpeAnnotateCounts tool help
	BedpeAnnotateCounts (2025_01-25-g1d2b52ea)
	
	Annotates a BEDPE file with NGSD count information of zipped BEDPE flat files.
	
	Mandatory parameters:
	  -in <file>                  Input BEDPE file.
	  -out <file>                 Output BEDPE file.
	  -ann_folder <file>          Input folder containing NGSD count flat files.
	
	Optional parameters:
	  -ps_name <string>           Processed sample name of the associated input file
	                              Default value: ''
	  -processing_system <string> Processing system short name of the processed sample
	                              Default value: ''
	  -disease_group <string>     Disease group of the input sample
	                              Default value: ''
	  -test                       Uses NGSD test db instead of the production db
	                              Default value: 'false'
	
	Special parameters:
	  --help                      Shows this help and exits.
	  --version                   Prints version and exits.
	  --changelog                 Prints changeloge and exits.
	  --tdx                       Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]           Settings override file (no other settings files are used).
	
### BedpeAnnotateCounts changelog
	BedpeAnnotateCounts 2025_01-25-g1d2b52ea
	
	2025-01-13 Added annotation of counts and AF grouped by disease group
	2022-02-11 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)