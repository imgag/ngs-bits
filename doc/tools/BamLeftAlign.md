### BamLeftAlign tool help
	BamLeftAlign (0.1-420-g3536bb0)
	
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
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamLeftAlign changelog
	BamLeftAlign 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)