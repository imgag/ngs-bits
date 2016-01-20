### GenesToBed tool help
	GenesToBed (0.1-184-gc4d2f1b)
	
	Converts a text file with gene names to a BED file.
	
	Mandatory parameters:
	  -source <enum> Transcript source database.
	                 Valid: 'ccds,ucsc'
	  -mode <enum>   Mode: gene = start/end of gene, exon = start/end of all exons of all splice variants.
	                 Valid: 'gene,exon'
	
	Optional parameters:
	  -in <file>     Input TXT file with one gene symbol per line. If unset, reads from STDIN.
	                 Default value: ''
	  -out <file>    Output BED file. If unset, writes to STDOUT.
	                 Default value: ''
	  -test          Uses the test database instead of on the production database.
	                 Default value: 'false'
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --tdx          Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)