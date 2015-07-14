### VcfLeftAlign tool help
	VcfLeftAlign (0.1-33-gfdb89ad)
	
	Shifts indels in a variant list as far to the left as possible. Complex indels and multi-allelic deletions are not shifted!
	
	Mandatory parameters:
	  -in <file>  Input variant list.
	  -out <file> Output variant list.
	
	Optional parameters:
	  -ref <file> Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	              Default value: ''
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits]("https://github.com/marc-sturm/ngs-bits")