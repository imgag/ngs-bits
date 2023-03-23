### NGSDImportEnsembl tool help
	NGSDImportEnsembl (2023_02-56-g0fe5818f)
	
	Imports Ensembl/CCDS transcript information into NGSD.
	
	Mandatory parameters:
	  -in <file>              Ensembl transcript file (download and unzip https://ftp.ensembl.org/pub/release-109/gff3/homo_sapiens/Homo_sapiens.GRCh38.109.gff3.gz).
	
	Optional parameters:
	  -pseudogenes <filelist> Pseudogene flat file(s) (download from http://pseudogene.org/psidr/psiDR.v0.txt and http://pseudogene.org/psicube/data/gencode.v10.pgene.parents.txt).
	                          Default value: ''
	  -all                    If set, all transcripts are imported (the default is to skip transcripts that do not have at least one of the flags 'GENCODE basic', 'Ensembl canonical', 'MANE select' or 'MANE plus clinical').
	                          Default value: 'false'
	  -test                   Uses the test database instead of on the production database.
	                          Default value: 'false'
	  -force                  If set, overwrites old data.
	                          Default value: 'false'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportEnsembl changelog
	NGSDImportEnsembl 2023_02-56-g0fe5818f
	
	2023-03-22 Removed parameters 'ensembl_canonical' and 'mane' as the information is now contained in the Ensembl GFF3 file.
	2022-10-17 Added transcript versions.
	2022-05-29 Added parameters 'ensembl_canonical' and 'mane'.
	2021-06-09 Added support for multiple pseudogene files and duplication check.
	2021-01-25 Made pseudogene file optional
	2021-01-20 Added import of pseudogene relations
	2019-08-12 Added handling of HGNC identifiers to resolve ambiguous gene names
	2017-07-06 Added first version
[back to ngs-bits](https://github.com/imgag/ngs-bits)