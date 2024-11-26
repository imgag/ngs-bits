### VcfAdd tool help
	VcfAdd (2024_08-110-g317f43b9)
	
	Appends variants from a VCF file to another VCF file.
	
	VCF header lines are taken from 'in' only.
	
	Mandatory parameters:
	  -in2 <file>           Input VCF file that is added to 'in'.
	
	Optional parameters:
	  -in <file>            Input VCF file to add 'in2' to.
	                        Default value: ''
	  -out <file>           Output VCF file with variants from 'in' and 'in2'.
	                        Default value: ''
	  -filter <string>      Tag variants from 'in2' with this filter entry.
	                        Default value: ''
	  -filter_desc <string> Description used in the filter header - use underscore instead of spaces.
	                        Default value: ''
	  -skip_duplicates      Skip variants from  'in2' which are also contained in 'in'.
	                        Default value: 'false'
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]     Settings override file (no other settings files are used).
	
### VcfAdd changelog
	VcfAdd 2024_08-110-g317f43b9
	
	2022-12-08 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)