### BedAnnotateFreq tool help
	BedAnnotateFreq (0.1-33-gfdb89ad)
	
	Extracts base frequencies for given regions from BAMs files.
	
	Mandatory parameters:
	  -bam <filelist> Input BAM file(s).
	
	Optional parameters:
	  -in <file>      Input BED file. If unset, reads from STDIN.
	                  Default value: ''
	  -out <file>     Output TSV file. If unset, writes to STDOUT.
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --tdx           Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits]("https://github.com/marc-sturm/ngs-bits")