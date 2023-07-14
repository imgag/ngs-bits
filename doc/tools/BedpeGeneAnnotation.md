### BedpeGeneAnnotation tool help
	BedpeGeneAnnotation (2023_03-63-gec44de43)
	
	Annotates a BEDPE file with gene information from the NGSD.
	
	Mandatory parameters:
	  -in <file>              Input BEDPE file containing the SVs.
	  -out <file>             Output BEDPE file containing the annotated SVs.
	
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
	
### BedpeGeneAnnotation changelog
	BedpeGeneAnnotation 2023_03-63-gec44de43
	
	2023-05-10 Added column with gene names at breakpoints.
	2020-01-27 Bugfix: 0-based BEDPE positions are now converted into 1-based BED positions.
	2020-01-21 Added ability to reannotate BEDPE files by overwriting old annotation.
	2020-01-20 Updated overlap method, refactored code.
	2020-01-14 Added handling of duplicates.
	2020-01-08 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)