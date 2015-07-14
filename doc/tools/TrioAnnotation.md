### TrioAnnotation tool help
	TrioAnnotation (0.1-33-gfdb89ad)
	
	Annotates a child sample with trio information. Assumes that the child is the index patient and that the parents are unaffected.
	
	Mandatory parameters:
	  -in <file>       Input variant list of child in TSV format.
	  -out <file>      Output file in TSV format.
	
	Optional parameters:
	  -min_depth <int> Minimum depth in all three samples.
	                   Default value: '10'
	  -max_maf <float> Maximum minor allele frequency in 1000G,ExAC and ESP6500EA database annotation.
	                   Default value: '0.01'
	  -max_ngsd <int>  Maximum homozygous occurances in NGSD.
	                   Default value: '30'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --tdx            Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits]("https://github.com/marc-sturm/ngs-bits")