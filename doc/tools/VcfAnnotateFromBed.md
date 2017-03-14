### VcfAnnotateFromBed tool help
	VcfAnnotateFromBed (0.1-645-gfc628cd)
	
	Annotates the INFO column of a VCF with data from a BED file.
	
	Mandatory parameters:
	  -bed <file>    BED file used for annotation.
	  -name <string> Annotation name in output VCF file.
	
	Optional parameters:
	  -in <file>     Input VCF file. If unset, reads from STDIN.
	                 Default value: ''
	  -out <file>    Output VCF list. If unset, writes to STDOUT.
	                 Default value: ''
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfAnnotateFromBed changelog
	VcfAnnotateFromBed 0.1-645-gfc628cd
	
	2017-03-14 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)