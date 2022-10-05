### TsvToQC tool help
	TsvToQC (2022_07-183-g2dc8c6f8)
	
	Converts TSV file to a qcML file.
	
	Mandatory parameters:
	  -sources <filelist> Source files the QC terms were extracted from.
	
	Optional parameters:
	  -in <file>          Input TSV file with two columns (QC term accession and value). If unset, reads from STDIN.
	                      Default value: ''
	  -out <file>         Output qcML file. If unset, writes to STDOUT.
	                      Default value: ''
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### TsvToQC changelog
	TsvToQC 2022_07-183-g2dc8c6f8
	
	2022-09-20 Initial inplementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)