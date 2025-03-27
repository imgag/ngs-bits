### BedpeAnnotateFromBed tool help
	BedpeAnnotateFromBed (2024_08-110-g317f43b9)
	
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
	  -no_duplicates       Remove duplicate annotations if several intervals from 'bed' overlap.
	                       Default value: 'false'
	  -url_decode          Decode URL encoded characters.
	                       Default value: 'false'
	  -replace_underscore  Replaces underscores with spaces in the annotation column.
	                       Default value: 'false'
	  -max_value           Select maximum value if several intervals from 'bed' overlap. (only for numeric columns)
	                       Default value: 'false'
	  -only_breakpoints    Only annotate overlaps with the confidence intervall of the break points.
	                       Default value: 'false'
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]    Settings override file (no other settings files are used).
	
### BedpeAnnotateFromBed changelog
	BedpeAnnotateFromBed 2024_08-110-g317f43b9
	
	2022-02-17 Added 'max_value' parameter.
	2020-01-27 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)