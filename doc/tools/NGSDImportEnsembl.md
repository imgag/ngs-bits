### NGSDImportEnsembl tool help
	NGSDImportEnsembl (0.1-773-g0c7504d)
	
	Imports Ensembl/CCDS transcript information into NGSD.
	
	Mandatory parameters:
	  -in <file>   Ensemble transcript file (download and unzip ftp://ftp.ensembl.org/pub/grch37/update/gff3/homo_sapiens/Homo_sapiens.GRCh37.87.gff3.gz).
	
	Optional parameters:
	  -all         If set, all transcripts are imported (the default is to skip transcripts not labeled as with the 'GENCODE basic' tag).
	               Default value: 'false'
	  -test        Uses the test database instead of on the production database.
	               Default value: 'false'
	  -force       If set, overwrites old data.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportEnsembl changelog
	NGSDImportEnsembl 0.1-773-g0c7504d
	
	2017-07-06 Added first version
[back to ngs-bits](https://github.com/imgag/ngs-bits)