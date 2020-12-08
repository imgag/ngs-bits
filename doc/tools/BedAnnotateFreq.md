### BedAnnotateFreq tool help
	BedAnnotateFreq (2020_09-79-gc6314b21)
	
	Extracts base frequencies for given regions from BAM/CRAM files.
	
	Mandatory parameters:
	  -bam <filelist> Input BAM/CRAM file(s).
	
	Optional parameters:
	  -in <file>      Input BED file. If unset, reads from STDIN.
	                  Default value: ''
	  -out <file>     Output TSV file. If unset, writes to STDOUT.
	                  Default value: ''
	  -ref <string>   Reference genome for CRAM support (mandatory if CRAM is used).
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedAnnotateFreq changelog
	BedAnnotateFreq 2020_09-79-gc6314b21
	
	2020-11-27 Added CRAM support.
[back to ngs-bits](https://github.com/imgag/ngs-bits)