### NGSDInit tool help
	NGSDInit (0.1-184-gc4d2f1b)
	
	Sets up the NDSD database (creates tables and adds minimal data).
	
	Optional parameters:
	  -add <file>     Additional SQL script to execute after database initialization.
	                  Default value: ''
	  -force <string> Database password needed to re-initialize the production database.
	                  Default value: ''
	  -test           Uses the test database instead of on the production database.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --tdx           Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)