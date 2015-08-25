### BamLeftAlign tool help
	BamLeftAlign (0.1-52-g9f9161f)
	
	Iteratively left-aligns and merges the insertions and deletions in all alignments.
	
	Mandatory parameters:
	  -in <file>      Input BAM file.
	  -out <file>     Output BAM file.
	
	Optional parameters:
	  -ref <file>     Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                  Default value: ''
	  -max_iter <int> Maximum number of iterations per read.
	                  Default value: '50'
	  -v              Verbose mode: Prints changed/unstable alignment names to the command line.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --tdx           Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)