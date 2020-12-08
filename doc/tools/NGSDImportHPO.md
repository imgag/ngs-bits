### NGSDImportHPO tool help
	NGSDImportHPO (2020_03-159-g5c8b2e82)
	
	Imports HPO terms and gene-phenotype relations into the NGSD.
	
	Mandatory parameters:
	  -obo <file>     HPO ontology file from 'http://purl.obolibrary.org/obo/hp.obo'.
	  -anno <file>    HPO annotations file from 'http://compbio.charite.de/jenkins/job/hpo.annotations/lastSuccessfulBuild/artifact/util/annotation/phenotype_to_genes.txt'
	
	Optional parameters:
	  -omim <file>    OMIM 'morbidmap.txt' file for additional disease-gene information, from 'https://omim.org/downloads/'.
	                  Default value: ''
	  -clinvar <file> ClinVar VCF file for additional disease-gene information. Download and unzip from 'ftp://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh37/archive_2.0/2020/clinvar_20200506.vcf.gz'.
	                  Default value: ''
	  -hgmd <file>    HGMD phenobase file (Manually download and unzip 'hgmd_phenbase-2020.2.dump').
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
	NGSDImportHPO 2020_03-159-g5c8b2e82
	
	2020-07-07 Added support of HGMD gene-phenotype relations.
	2020-07-06 Added support for HGMD phenobase file.
	2020-03-10 Removed support for old HPO annotation file.
	2020-03-09 Added optimization for hpo-gene relations.
	2020-03-05 Added support for new HPO annotation file.
[back to ngs-bits](https://github.com/imgag/ngs-bits)