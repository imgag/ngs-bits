### BamClipOverlap tool help
	BamClipOverlap (0.1-606-g0e1e89d)
	
	Softclipping of overlapping reads.
	
	Overlapping reads will be soft-clipped from start to end.
	If mismatches are found in the overlap of both reads, the mapping quality of both reads will be set to zero.
	
	Mandatory parameters:
	  -in <file>   Input bam file. Needs to be sorted by name.
	  -out <file>  Output bam file.
	
	Optional parameters:
	  -amplicon    Amplicon mode: one read of a pair will be clipped randomly.
	               Default value: 'false'
	  -v           Verbose mode.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamClipOverlap changelog
	BamClipOverlap 0.1-606-g0e1e89d
	
	2017-01-16 Added overlap mismatch filter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)