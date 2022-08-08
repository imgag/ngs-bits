### NGSDImportHPO tool help
	NGSDImportHPO (2022_07-37-g22d4e20c)
	
	Imports HPO terms and gene-phenotype relations into the NGSD.
	
	Mandatory parameters:
	  -obo <file>      HPO ontology file from 'http://purl.obolibrary.org/obo/hp.obo'.
	  -anno <file>     HPO annotations file from 'http://purl.obolibrary.org/obo/hp/hpoa/phenotype_to_genes.txt'
	
	Optional parameters:
	  -omim <file>     OMIM 'morbidmap.txt' file for additional disease-gene information, from 'https://omim.org/downloads/'.
	                   Default value: ''
	  -clinvar <file>  ClinVar VCF file for additional disease-gene information. Download and unzip from 'https://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh37/archive_2.0/2022/clinvar_20220702.vcf.gz' for GRCH37 or 'http://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh38/archive_2.0/2021/clinvar_20211212.vcf.gz' for GRCh38.
	                   Default value: ''
	  -hgmd <file>     HGMD phenobase file (Manually download and unzip 'hgmd_phenbase-2022.2.dump').
	                   Default value: ''
	  -hpophen <file>  HPO 'phenotype.hpoa' file for additional phenotype-disease evidence information. Download from http://purl.obolibrary.org/obo/hp/hpoa/phenotype.hpoa
	                   Default value: ''
	  -gencc <file>    gencc 'gencc-submissions.csv' file for additional disease-gene evidence information. Download from https://search.thegencc.org/download
	                   Default value: ''
	  -decipher <file> G2P 'DDG2P.csv' file for additional gene-disease-phenotype evidence information. Download from https://www.deciphergenomics.org/about/downloads/data
	                   Default value: ''
	  -test            Uses the test database instead of on the production database.
	                   Default value: 'false'
	  -force           If set, overwrites old data.
	                   Default value: 'false'
	  -debug           Enables debug output
	                   Default value: 'false'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --changelog      Prints changeloge and exits.
	  --tdx            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportHPO changelog
	NGSDImportHPO 2022_07-37-g22d4e20c
	
	2021-12-22 Added support for GenCC and DECIPHER.
	2020-07-07 Added support of HGMD gene-phenotype relations.
	2020-07-06 Added support for HGMD phenobase file.
	2020-03-10 Removed support for old HPO annotation file.
	2020-03-09 Added optimization for hpo-gene relations.
	2020-03-05 Added support for new HPO annotation file.
[back to ngs-bits](https://github.com/imgag/ngs-bits)