### NGSDImportHPO tool help
	NGSDImportHPO (2024_06-33-g1f38e35e)
	
	Imports HPO terms and gene-phenotype relations into the NGSD.
	
	Mandatory parameters:
	  -obo <file>      HPO ontology file from 'https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2024-08-13/hp.obo'.
	  -anno <file>     HPO annotations file from 'https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2024-08-13/phenotype_to_genes.txt'
	
	Optional parameters:
	  -omim <file>     OMIM 'morbidmap.txt' file for additional disease-gene information, from 'https://omim.org/downloads/'.
	                   Default value: ''
	  -clinvar <file>  ClinVar VCF file for additional disease-gene information. Download and unzip from 'https://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh38/archive_2.0/2024/clinvar_20240805.vcf.gz'.
	                   Default value: ''
	  -hgmd <file>     HGMD phenobase file (Manually download and unzip 'hgmd_phenbase-2024.2.dump').
	                   Default value: ''
	  -hpophen <file>  HPO 'phenotype.hpoa' file for additional phenotype-disease evidence information. Download from wget https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2024-08-13/phenotype.hpoa
	                   Default value: ''
	  -gencc <file>    gencc 'gencc-submissions.csv' file for additional disease-gene evidence information. Download from https://search.thegencc.org/download
	                   Default value: ''
	  -decipher <file> G2P 'DDG2P.csv' file for additional gene-disease-phenotype evidence information. Download from https://www.ebi.ac.uk/gene2phenotype/downloads/
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
	NGSDImportHPO 2024_06-33-g1f38e35e
	
	2021-12-22 Added support for GenCC and DECIPHER.
	2020-07-07 Added support of HGMD gene-phenotype relations.
	2020-07-06 Added support for HGMD phenobase file.
	2020-03-10 Removed support for old HPO annotation file.
	2020-03-09 Added optimization for hpo-gene relations.
	2020-03-05 Added support for new HPO annotation file.
[back to ngs-bits](https://github.com/imgag/ngs-bits)