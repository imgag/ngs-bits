### BamClipOverlap tool help
	BamClipOverlap (0.1-46-gb124721)
	
	Softclipping of overlapping reads.
	
	Mandatory parameters:
	  -in <file>  Input bam file. Needs to be sorted by name.
	  -out <file> Output bam file.
	
	Optional parameters:
	  -amplicon   Amplicon mode: one read of a pair will be clipped randomly.
	              Default value: 'false'
	  -v          Verbose mode.
	              Default value: 'false'
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)