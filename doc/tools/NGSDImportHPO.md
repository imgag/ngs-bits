### NGSDImportHPO tool help
	NGSDImportHPO (0.1-184-gc4d2f1b)
	
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
	  --tdx        Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)