### NGSDImportHGNC tool help
	NGSDImportHGNC (2022_04-120-g0b2ddab9)
	
	Imports genes from the HGNC flat file.
	
	Mandatory parameters:
	  -in <file>      HGNC flat file (download ftp://ftp.ebi.ac.uk/pub/databases/genenames/hgnc/tsv/hgnc_complete_set.txt)
	  -ensembl <file> Ensembl gene file (gff3) to resolve duplicate ENSG identifier.
	
	Optional parameters:
	  -test           Uses the test database instead of on the production database.
	                  Default value: 'false'
	  -force          If set, overwrites old data.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportHGNC changelog
	NGSDImportHGNC 2022_04-120-g0b2ddab9
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)