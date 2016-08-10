### NGSDImportHPO tool help
	NGSDImportHPO (0.1-420-g3536bb0)
	
	Imports HPO terms and gene-phenotype correlations into NGSD.
	
	Mandatory parameters:
	  -obo <file>  HPO ontology file 'hp.obo' from 'http://purl.obolibrary.org/obo/'.
	  -gene <file> HPO phenotype-gene relation file 'ALL_SOURCES_ALL_FREQUENCIES_diseases_to_genes_to_phenotypes.txt' from 'http://compbio.charite.de/jenkins/job/hpo.annotations.monthly/lastStableBuild/artifact/annotation/'
	
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
	NGSDImportHPO 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)