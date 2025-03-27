### TsvToQC tool help
	TsvToQC (2024_08-110-g317f43b9)
	
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
	  --settings [file]   Settings override file (no other settings files are used).
	
### TsvToQC changelog
	TsvToQC 2024_08-110-g317f43b9
	
	2022-09-20 Initial inplementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)