### GenesToApproved tool help
	GenesToApproved (2019_08-14-g8b6a17a)
	
	Replaces gene symbols by approved symbols using the HGNC database.
	
	Optional parameters:
	  -in <file>         Input TXT file with one gene symbol per line. If unset, reads from STDIN.
	                     Default value: ''
	  -out <file>        Output TXT file with approved gene symbols. If unset, writes to STDOUT.
	                     Default value: ''
	  -test              Uses the test database instead of on the production database.
	                     Default value: 'false'
	  -report_ambiguous  Report all matching genes for ambiguous previous/synonymous symbols - instead of an error.
	                     Default value: 'false'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### GenesToApproved changelog
	GenesToApproved 2019_08-14-g8b6a17a
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)