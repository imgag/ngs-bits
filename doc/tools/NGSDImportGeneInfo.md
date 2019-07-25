### NGSDImportGeneInfo tool help
	NGSDImportGeneInfo (2019_07-8-g5a9c094)
	
	Imports gene-specific information into NGSD.
	
	Mandatory parameters:
	  -constraint <file> gnomAD gene contraint file (download and unzip https://storage.googleapis.com/gnomad-public/release/2.1.1/constraint/gnomad.v2.1.1.lof_metrics.by_gene.txt.bgz).
	
	Optional parameters:
	  -test              Uses the test database instead of on the production database.
	                     Default value: 'false'
	  -force             If set, overwrites old data.
	                     Default value: 'false'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportGeneInfo changelog
	NGSDImportGeneInfo 2019_07-8-g5a9c094
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)