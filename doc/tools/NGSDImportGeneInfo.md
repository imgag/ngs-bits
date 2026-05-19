### NGSDImportGeneInfo tool help
	NGSDImportGeneInfo (2025_12-266-g396e1fe11)
	
	Imports gene-specific information into NGSD.
	
	Mandatory parameters:
	  -constraint <file> gnomAD gene contraint file (download and unzip https://storage.googleapis.com/gcp-public-data--gnomad/release/4.1.1/constraint/gnomad.v4.1.1.constraint_metrics.tsv.bgz).
	
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
	  --settings [file]  Settings override file (no other settings files are used).
	
### NGSDImportGeneInfo changelog
	NGSDImportGeneInfo 2025_12-266-g396e1fe11
	
	2026-04-17 Update to gnomAD 4.1.1 constraints.
	2019-07-16 Update to gnomAD 2.1.1 constraints.
	2016-11-23 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)