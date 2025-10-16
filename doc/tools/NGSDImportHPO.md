### NGSDImportHPO tool help
	NGSDImportHPO (2025_07-127-g60fc6b39)
	
	Imports HPO terms and gene-phenotype relations into the NGSD.
	
	Mandatory parameters:
	  -obo <file>       HPO ontology file from https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2025-09-01/hp.obo
	  -anno <file>      HPO annotations file from https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2025-09-01/phenotype_to_genes.txt
	
	Optional parameters:
	  -omim <file>      OMIM 'morbidmap.txt' file for additional disease-gene information, from https://omim.org/downloads/
	                    Default value: ''
	  -clinvar <file>   ClinVar VCF file for additional disease-gene information. Download and unzip from https://ftp.ncbi.nlm.nih.gov/pub/clinvar/vcf_GRCh38/archive_2.0/2025/clinvar_20250907.vcf.gz
	                    Default value: ''
	  -hgmd <file>      HGMD phenbase file (Manually download 'hgmd_phenbase-2025.2.dump.gz').
	                    Default value: ''
	  -hpophen <file>   HPO 'phenotype.hpoa' file for additional phenotype-disease evidence information. Download from https://github.com/obophenotype/human-phenotype-ontology/releases/download/v2025-09-01/phenotype.hpoa
	                    Default value: ''
	  -gencc <file>     gencc 'gencc-submissions.csv' file for additional disease-gene evidence information. (Manually download from https://search.thegencc.org/download).
	                    Default value: ''
	  -g2p <file>       DDG2P file for additional gene-disease-phenotype evidence information. Download from http://ftp.ebi.ac.uk/pub/databases/gene2phenotype/G2P_data_downloads/2025_08_28/DDG2P_2025-08-28.csv.gz
	                    Default value: ''
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	  -force            If set, overwrites old data.
	                    Default value: 'false'
	  -debug            Enables debug output
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDImportHPO changelog
	NGSDImportHPO 2025_07-127-g60fc6b39
	
	2025-09-23 Renamed '-decipher' to '-g2p' and adapted parser to new G2P format.
	2021-12-22 Added support for GenCC and DECIPHER.
	2020-07-07 Added support of HGMD gene-phenotype relations.
	2020-07-06 Added support for HGMD phenbase file.
	2020-03-10 Removed support for old HPO annotation file.
	2020-03-09 Added optimization for hpo-gene relations.
	2020-03-05 Added support for new HPO annotation file.
[back to ngs-bits](https://github.com/imgag/ngs-bits)