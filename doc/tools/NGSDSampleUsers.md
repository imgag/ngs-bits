### NGSDSampleUsers tool help
	NGSDSampleUsers (2026_06-46-g72aab0308)
	
	Returns a list of users that evaluated a sample.
	
	Optional parameters:
	  -in <file>        Input TSV file with processed sample IDs in first column. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDSampleUsers changelog
	NGSDSampleUsers 2026_06-46-g72aab0308
	
	2026-06-16 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)