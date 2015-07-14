### BedGeneOverlap tool help
	BedGeneOverlap (0.1-33-gfdb89ad)
	
	Calculates how much of each overlapping gene is covered.
	
	Optional parameters:
	  -in <file>  Input BED file. If unset, reads from STDIN.
	              Default value: ''
	  -out <file> Output TSV file. If unset, writes to STDOUT.
	              Default value: ''
	  -db <file>  The database file to use. A BED file containing all exons with gene names. If unset 'ccds_merged' from the 'settings.ini' file is used.
	              Default value: ''
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits]("https://github.com/marc-sturm/ngs-bits")