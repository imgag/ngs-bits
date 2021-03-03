### NGSDImportEnsembl tool help
	NGSDImportEnsembl (2020_12-52-gd0b78e6c)
	
	Imports Ensembl/CCDS transcript information into NGSD.
	
	Mandatory parameters:
	  -in <file>          Ensembl transcript file (download and unzip ftp://ftp.ensembl.org/pub/grch37/release-87/gff3/homo_sapiens/Homo_sapiens.GRCh37.87.chr.gff3.gz).
	
	Optional parameters:
	  -pseudogenes <file> Pseudogene flat file (download from http://pseudogene.org/psidr/psiDR.v0.txt).
	                      Default value: ''
	  -all                If set, all transcripts are imported (the default is to skip transcripts not labeled as with the 'GENCODE basic' tag).
	                      Default value: 'false'
	  -test               Uses the test database instead of on the production database.
	                      Default value: 'false'
	  -force              If set, overwrites old data.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportEnsembl changelog
	NGSDImportEnsembl 2020_12-52-gd0b78e6c
	
	2021-01-25 Made pseudogene file optional
	2021-01-20 Added import of pseudogene relations
	2019-08-12 Added handling of HGNC identifiers to resolve ambiguous gene names
	2017-07-06 Added first version
[back to ngs-bits](https://github.com/imgag/ngs-bits)