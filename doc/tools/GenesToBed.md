### GenesToBed tool help
	GenesToBed (0.1-52-g9f9161f)
	
	Converts a text file with gene names to a BED file.
	
	Mandatory parameters:
	  -in <file>      Input TXT file with one gene symbol per line.
	  -mode <enum>    Mode: gene = the gene in UCSC, splice = all splice variants in UCSC, exon = all coding exons of all splice variants in UCSC, ccds = all coding exons of all splice variants in CCDS.
	                  Valid: 'gene,splice,exon,ccds'
	
	Optional parameters:
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	  -db_ccds <file> The CCDS flat file. If unset 'ccds_joined' from the 'settings.ini' file is used.
	                  Default value: ''
	  -db_ucsc <file> The UCSC flat file. If unset 'kgxref_joined' from the 'settings.ini' file is used.
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --tdx           Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)