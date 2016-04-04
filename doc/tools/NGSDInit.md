### NGSDInit tool help
	NGSDInit (0.1-222-g9be2128)
	
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
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDInit changelog
	NGSDInit 0.1-222-g9be2128
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)