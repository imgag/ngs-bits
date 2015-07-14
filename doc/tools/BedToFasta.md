### BedToFasta tool help
	BedToFasta (0.1-33-gfdb89ad)
	
	Converts a BED file to a FASTA file.
	
	Mandatory parameters:
	  -in <file>  Input BED file.
	
	Optional parameters:
	  -out <file> Output FASTA file. If unset, writes to STDOUT.
	              Default value: ''
	  -ref <file> Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	              Default value: ''
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits]("https://github.com/marc-sturm/ngs-bits")