### BedAnnotateFromBed tool help
	BedAnnotateFromBed (2019_05-36-gb3caeea)
	
	Annotates BED file regions with information from a second BED file.
	
	Mandatory parameters:
	  -in2 <file>     BED file that is used as annotation source.
	
	Optional parameters:
	  -in <file>      Input BED file. If unset, reads from STDIN.
	                  Default value: ''
	  -out <file>     Output BED file. If unset, writes to STDOUT.
	                  Default value: ''
	  -col <int>      Annotation source column (if column number does not exist, 'yes' is used).
	                  Default value: '4'
	  -clear          Clear all annotations present in the 'in' file.
	                  Default value: 'false'
	  -no_duplicates  Remove duplicate annotations if several intervals from 'in2' overlap.
	                  Default value: 'false'
	  -overlap        Annotate percentage of overlap. The regular annotation is appended in brackets.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedAnnotateFromBed changelog
	BedAnnotateFromBed 2019_05-36-gb3caeea
	
	2019-07-09 Added parameters 'col' and 'no_duplicates'; Fixed 'clear' parameter.
	2017-11-28 Added 'clear' flag.
	2017-11-03 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)