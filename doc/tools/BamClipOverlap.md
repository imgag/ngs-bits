### BamClipOverlap tool help
	BamClipOverlap (0.1-190-g94e4c3d)
	
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
	  --tdx       Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)