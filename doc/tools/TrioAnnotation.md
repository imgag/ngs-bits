### TrioAnnotation tool help
	TrioAnnotation (0.1-873-g04f7bd6)
	
	Annotates a child sample with trio information. Assumes that the child is the index patient and that the parents are unaffected.
	
	Mandatory parameters:
	  -in <file>       Input variant list of child in TSV format.
	  -out <file>      Output file in TSV format.
	  -gender <enum>   Gender of the child.
	                   Valid: 'male,female'
	
	Optional parameters:
	  -min_depth <int> Minimum depth in all three samples.
	                   Default value: '10'
	  -max_maf <float> Maximum minor allele frequency in 1000G,ExAC and gnomAD database annotation.
	                   Default value: '0.01'
	  -max_ngsd <int>  Maximum homozygous occurances in NGSD.
	                   Default value: '30'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --changelog      Prints changeloge and exits.
	  --tdx            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### TrioAnnotation changelog
	TrioAnnotation 0.1-873-g04f7bd6
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)