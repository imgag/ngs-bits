### NGSDExportSV tool help
	NGSDExportSV (2021_12-170-g1acf8802)
	
	Exports all SVs from the NGSD into BEDPE files.
	
	Mandatory parameters:
	  -out_folder <file> Output folder for the exported BEDPE files.
	
	Optional parameters:
	  -test              Uses the test database instead of on the production database.
	                     Default value: 'false'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDExportSV changelog
	NGSDExportSV 2021_12-170-g1acf8802
	
	2022-02-24 Changed SV break point output format.
	2022-02-18 Implemented tool.
	2022-02-10 Initial struct.
[back to ngs-bits](https://github.com/imgag/ngs-bits)