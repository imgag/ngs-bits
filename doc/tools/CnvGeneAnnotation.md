### CnvGeneAnnotation tool help
	CnvGeneAnnotation (2024_08-110-g317f43b9)
	
	Annotates TSV file containing CNVs with gene information from NGSD.
	
	Mandatory parameters:
	  -in <file>              Input TSV file containing the CNVs.
	  -out <file>             Output TSV file containing the annotated CNVs.
	
	Optional parameters:
	  -add_simple_gene_names  Adds an additional column containing only the list of gene names.
	                          Default value: 'false'
	  -test                   Uses the test database instead of on the production database.
	                          Default value: 'false'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]       Settings override file (no other settings files are used).
	
### CnvGeneAnnotation changelog
	CnvGeneAnnotation 2024_08-110-g317f43b9
	
	2019-11-11 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)