### NGSDImportHPO tool help
	NGSDImportHPO (0.1-222-g9be2128)
	
	Imports HPO terms and gene-phenotype correlations into NGSD (download from http://hgdownload.cse.ucsc.edu/goldenPath/hg19/database/).
	
	Mandatory parameters:
	  -obo <file>  HPO ontology file 'hp.obo'.
	  -gene <file> HPO phenotype-gene relation file 'ALL_SOURCES_ALL_FREQUENCIES_diseases_to_genes_to_phenotypes.txt'
	
	Optional parameters:
	  -test        Uses the test database instead of on the production database.
	               Default value: 'false'
	  -force       If set, overwrites old data.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportHPO changelog
	NGSDImportHPO 0.1-222-g9be2128
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)