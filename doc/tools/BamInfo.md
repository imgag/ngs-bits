### BamInfo tool help
	BamInfo (2025_07-127-g60fc6b39)
	
	Basic BAM information.
	
	Mandatory parameters:
	  -in <filelist>    Input BAM/CRAM files.
	
	Optional parameters:
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -name             Add filename only to output. The default is to add the canonical file path.
	                    Default value: 'false'
	  -ref <file>       Reference genome for CRAM support (mandatory if CRAM is used).
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BamInfo changelog
	BamInfo 2025_07-127-g60fc6b39
	
	2025-09-06 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)