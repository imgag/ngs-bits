### NGSDImportEnsembl tool help
	NGSDImportEnsembl (2022_10-83-g62451d12)
	
	Imports Ensembl/CCDS transcript information into NGSD.
	
	Mandatory parameters:
	  -in <file>                Ensembl transcript file (download and unzip https://ftp.ensembl.org/pub/grch37/release-87/gff3/homo_sapiens/Homo_sapiens.GRCh37.87.chr.gff3.gz for GRCh37 and https://ftp.ensembl.org/pub/release-107/gff3/homo_sapiens/Homo_sapiens.GRCh38.107.chr.gff3.gz for GRCh38).
	  -ensembl_canonical <file> Ensembl canonical transcript TSV file (download and unzip from https://ftp.ensembl.org/pub/release-105/tsv/homo_sapiens/Homo_sapiens.GRCh38.105.canonical.tsv.gz
	  -mane <file>              GFF file with MANE information (download and unzip from https://ftp.ncbi.nlm.nih.gov/refseq/MANE/MANE_human/release_1.0/MANE.GRCh38.v1.0.ensembl_genomic.gff.gz
	
	Optional parameters:
	  -pseudogenes <filelist>   Pseudogene flat file(s) (download from http://pseudogene.org/psidr/psiDR.v0.txt and http://pseudogene.org/psicube/data/gencode.v10.pgene.parents.txt).
	                            Default value: ''
	  -all                      If set, all transcripts are imported (the default is to skip transcripts that do not have at least one of the flags 'GENCODE basic', 'Ensembl canonical', 'MANE select' or 'MANE plus clinical').
	                            Default value: 'false'
	  -test                     Uses the test database instead of on the production database.
	                            Default value: 'false'
	  -force                    If set, overwrites old data.
	                            Default value: 'false'
	
	Special parameters:
	  --help                    Shows this help and exits.
	  --version                 Prints version and exits.
	  --changelog               Prints changeloge and exits.
	  --tdx                     Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportEnsembl changelog
	NGSDImportEnsembl 2022_10-83-g62451d12
	
	2022-10-17 Added transcript versions.
	2022-05-29 Added parameters 'ensembl_canonical' and 'mane'.
	2021-06-09 Added support for multiple pseudogene files and duplication check.
	2021-01-25 Made pseudogene file optional
	2021-01-20 Added import of pseudogene relations
	2019-08-12 Added handling of HGNC identifiers to resolve ambiguous gene names
	2017-07-06 Added first version
[back to ngs-bits](https://github.com/imgag/ngs-bits)