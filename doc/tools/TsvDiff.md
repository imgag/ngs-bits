### TsvDiff tool help
	TsvDiff (2024_11-41-g79ac725e)
	
	Compares TSV files.
	
	Mandatory parameters:
	  -in1 <file>                      First input TSV file.
	  -in2 <file>                      Second input TSV file.
	
	Optional parameters:
	  -out <file>                      Output file with differences. If unset, writes to stdout.
	                                   Default value: ''
	  -skip_comments                   Do not compare comment lines starting with '##'.
	                                   Default value: 'false'
	  -skip_comments_matching <string> Comma-separated list of sub-strings for skipping comment lines (case-sensitive matching).
	                                   Default value: ''
	  -skip_cols <string>              Comma-separated list of colums to skip.
	                                   Default value: ''
	  -no_error                        Do not exit with error state if differences are detected
	                                   Default value: 'false'
	  -debug                           Print debug output to stderr
	                                   Default value: 'false'
	
	Special parameters:
	  --help                           Shows this help and exits.
	  --version                        Prints version and exits.
	  --changelog                      Prints changeloge and exits.
	  --tdx                            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]                Settings override file (no other settings files are used).
	
### TsvDiff changelog
	TsvDiff 2024_11-41-g79ac725e
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)