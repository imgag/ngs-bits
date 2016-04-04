### Cidx tool help
	Cidx (0.1-222-g9be2128)
	
	Indexes and searches tab-separated chromosomal database files (1-based positions).
	
	Mandatory parameters:
	  -in <file>    Input database file.
	
	Optional parameters:
	  -bed <file>   Chromosomal ranges file for which the database content is retrieved.
	                Default value: ''
	  -pos <string> Chromosomal range in format 'chr:start-end' for which the database content is retrieved.
	                Default value: ''
	  -out <file>   Output file. If unset, writes to STDOUT.
	                Default value: ''
	  -force        Forces database index update, even if it is up-to-date.
	                Default value: 'false'
	  -c <int>      Database file 0-based chromosome column index (Only needed to create index file).
	                Default value: '0'
	  -s <int>      Database file 0-based start position column index (Only needed to create index file).
	                Default value: '1'
	  -e <int>      Database file 0-based end position column index (Only needed to create index file).
	                Default value: '2'
	  -h <string>   Database file header comment character (Only needed to create index file).
	                Default value: '#'
	  -b <int>      Index bin size: An index entry is created for every n'th line in the database (Only needed to create index file).
	                Default value: '1000'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --changelog   Prints changeloge and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### Cidx changelog
	Cidx 0.1-222-g9be2128
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)