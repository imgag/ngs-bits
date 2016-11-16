### GenesToBed tool help
	GenesToBed (0.1-563-g5bbcca6)
	
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
	  -fallback      Allow fallback to all source databases, if not transcript for a gene is defined in the selected source database.
	                 Default value: 'false'
	  -test          Uses the test database instead of on the production database.
	                 Default value: 'false'
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### GenesToBed changelog
	GenesToBed 0.1-563-g5bbcca6
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)