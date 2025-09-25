### VcfAdd tool help
	VcfAdd (2025_07-127-g60fc6b39)
	
	Merges several VCF files into one VCF by appending one to the other.
	
	Variant lines from all other input files are appended to the first input file.
	VCF header lines are taken from the first input file only.
	
	Mandatory parameters:
	  -in <filelist>        Input files to merge in VCF or VCG.GZ format.
	
	Optional parameters:
	  -out <file>           Output VCF file with all variants.
	                        Default value: ''
	  -filter <string>      Tag variants from all but the first input file with this filter entry.
	                        Default value: ''
	  -filter_desc <string> Description used in the filter header - use underscore instead of spaces.
	                        Default value: ''
	  -skip_duplicates      Skip variants if they occur more than once.
	                        Default value: 'false'
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]     Settings override file (no other settings files are used).
	
### VcfAdd changelog
	VcfAdd 2025_07-127-g60fc6b39
	
	2025-01-17 Added support for gzipped VCFs and removing duplicates if there is only one input file.
	2022-12-08 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)