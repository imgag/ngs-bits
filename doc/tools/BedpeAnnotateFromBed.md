### BedpeAnnotateFromBed tool help
	BedpeAnnotateFromBed (2019_11-129-g4a5e7bf9)
	
	Annotates a BEDPE file with information from a BED file.
	
	Mandatory parameters:
	  -bed <file>          BED file that is used as annotation source.
	
	Optional parameters:
	  -in <file>           Input BEDPE file. If unset, reads from STDIN.
	                       Default value: ''
	  -out <file>          Output BEDPE file. If unset, writes to STDOUT.
	                       Default value: ''
	  -col <int>           Annotation source column (default: 4).
	                       Default value: '4'
	  -col_name <string>   Name of the annotated column
	                       Default value: 'ANNOTATION'
	  -no_duplicates       Remove duplicate annotations if several intervals from 'in2' overlap.
	                       Default value: 'false'
	  -url_decode          Decode URL encoded characters.
	                       Default value: 'false'
	  -replace_underscore  Replaces underscores with spaces in the annotation column.
	                       Default value: 'false'
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedpeAnnotateFromBed changelog
	BedpeAnnotateFromBed 2019_11-129-g4a5e7bf9
	
	2020-01-27 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)