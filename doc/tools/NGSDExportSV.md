### NGSDExportSV tool help
	NGSDExportSV (2025_01-25-g1d2b52ea)
	
	Exports all SVs from the NGSD into BEDPE files.
	
	Mandatory parameters:
	  -out_folder <file>          Output folder for the exported BEDPE files.
	
	Optional parameters:
	  -test                       Uses the test database instead of on the production database.
	                              Default value: 'false'
	  -common_sys_threshold <int> Minimal number of samples for which a seperate density file is created.
	                              Default value: '50'
	
	Special parameters:
	  --help                      Shows this help and exits.
	  --version                   Prints version and exits.
	  --changelog                 Prints changeloge and exits.
	  --tdx                       Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]           Settings override file (no other settings files are used).
	
### NGSDExportSV changelog
	NGSDExportSV 2025_01-25-g1d2b52ea
	
	2025-01-13 Added DISEASE_GROUP column to BEDPE files
	2024-02-07 Added output of processing specific breakpoint density.
	2022-02-24 Changed SV break point output format.
	2022-02-18 Implemented tool.
	2022-02-10 Initial struct.
[back to ngs-bits](https://github.com/imgag/ngs-bits)