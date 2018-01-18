### NGSDImportHPO tool help
	NGSDImportHPO (0.1-983-gfeaa1f3)
	
	Imports HPO terms and gene-phenotype relations into the NGSD.
	
	Mandatory parameters:
	  -obo <file>     HPO ontology file from 'http://purl.obolibrary.org/obo/hp.obo'.
	  -anno <file>    HPO annotations file from 'http://compbio.charite.de/jenkins/job/hpo.annotations.monthly/lastStableBuild/artifact/annotation/ALL_SOURCES_ALL_FREQUENCIES_diseases_to_genes_to_phenotypes.txt'
	
	Optional parameters:
	  -omim <file>    OMIM 'morbidmap.txt' file for additional disease-gene information, from 'https://omim.org/downloads/'.
	                  Default value: ''
	  -clinvar <file> ClinVar VCF file for additional disease-gene information, from 'ftp://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh37/archive_2.0/2018/clinvar_20171203.vcf.gz'.
	                  Default value: ''
	  -test           Uses the test database instead of on the production database.
	                  Default value: 'false'
	  -force          If set, overwrites old data.
	                  Default value: 'false'
	  -debug          Enables debug output
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportHPO changelog
	NGSDImportHPO 0.1-983-gfeaa1f3
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)