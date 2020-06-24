### BedpeAnnotateCnvOverlap tool help
	BedpeAnnotateCnvOverlap (2020_03-184-g27235379)
	
	Annotates a SV file with (high-quality) CNV overlap of a given file.
	
	Mandatory parameters:
	  -in <file>   Input SV file (in BEDPE format).
	  -out <file>  Output SV file (in BEDPE format).
	  -cnv <file>  Input CNV file (in TSV format).
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedpeAnnotateCnvOverlap changelog
	BedpeAnnotateCnvOverlap 2020_03-184-g27235379
	
	2020-06-19 Added CNV quality filter (loglikelihood).
	2020-06-18 Changed tool from reporting to annotation.
	2020-06-02 Initial version of the tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)