### NGSDImportGeneInfo tool help
	NGSDImportGeneInfo (0.1-568-ga0c0b35)
	
	Imports gene-specific information into NGSD.
	
	Mandatory parameters:
	  -constraint <file> ExAC gene contraint file from 'ftp://ftp.broadinstitute.org/pub/ExAC_release/current/functional_gene_constraint/'.
	
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
	NGSDImportGeneInfo 0.1-568-ga0c0b35
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)