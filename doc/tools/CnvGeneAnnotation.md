### CnvGeneAnnotation tool help
	CnvGeneAnnotation (2019_09-57-g68529808)
	
	Annotates a CNVs file with gene information from NGSD (gene constraint, gene region).
	
	Mandatory parameters:
	  -in <file>              Input TSV file containing the CNVs.
	  -out <file>             Output TSV file containing the annotated CNVs.
	
	Optional parameters:
	  -add_simple_gene_names  Adds an additioal column containing only the list of gene names.
	                          Default value: 'false'
	  -test                   Uses the test database instead of on the production database.
	                          Default value: 'false'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### CnvGeneAnnotation changelog
	CnvGeneAnnotation 2019_09-57-g68529808
	
	2019-11-11 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)