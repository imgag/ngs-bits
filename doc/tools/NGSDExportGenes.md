### NGSDExportGenes tool help
	NGSDExportGenes (2024_08-110-g317f43b9)
	
	Lists genes from NGSD.
	
	Optional parameters:
	  -out <file>        Output TSV file. If unset, writes to STDOUT.
	                     Default value: ''
	  -add_disease_info  Annotate with disease information from HPO, OrphaNet and OMIM (slow).
	                     Default value: 'false'
	  -test              Uses the test database instead of on the production database.
	                     Default value: 'false'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]  Settings override file (no other settings files are used).
	
### NGSDExportGenes changelog
	NGSDExportGenes 2024_08-110-g317f43b9
	
	2021-04-13 Added more information (imprinting, pseudogenes, OrphaNet, OMIM).
	2019-09-20 Added several columns with gene details.
	2018-05-03 First version
[back to ngs-bits](https://github.com/imgag/ngs-bits)