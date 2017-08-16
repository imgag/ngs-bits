### NGSDImportHPO tool help
	NGSDImportHPO (0.1-827-g472ac9f)
	
	Imports HPO terms and gene-phenotype relations into the NGSD.
	
	Mandatory parameters:
	  -obo <file>   HPO ontology file 'hp.obo' from 'http://purl.obolibrary.org/obo/hp.obo'.
	  -anno <file>  HPO annotations file 'phenotype_annotation.tab' from 'http://compbio.charite.de/jenkins/job/hpo.annotations/lastStableBuild/artifact/misc/phenotype_annotation.tab'
	  -genes <file> HPO genes file 'diseases_to_genes.txt' from 'http://compbio.charite.de/jenkins/job/hpo.annotations.monthly/lastStableBuild/artifact/annotation/diseases_to_genes.txt'
	
	Optional parameters:
	  -omim <file>  OMIM 'morbidmap.txt' file, from 'https://omim.org/downloads/'.
	                Default value: ''
	  -test         Uses the test database instead of on the production database.
	                Default value: 'false'
	  -force        If set, overwrites old data.
	                Default value: 'false'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --changelog   Prints changeloge and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportHPO changelog
	NGSDImportHPO 0.1-827-g472ac9f
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)