### SamplesNGSD tool help
	SamplesNGSD (0.1-33-gfdb89ad)
	
	Lists processed samples from NGSD.
	
	Optional parameters:
	  -out <file>       Output TSV file. If unset, writes to STDOUT.
	                    Default value: ''
	  -project <string> Filter for project name.
	                    Default value: ''
	  -sys <string>     Filter for processing system short name.
	                    Default value: ''
	  -quality <enum>   Minimum sample/run quality.
	                    Default value: 'good'
	                    Valid: 'bad,medium,good'
	  -normal           If set, tumor samples are excluded.
	                    Default value: 'false'
	  -check_path       Checks the sample folder location.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --tdx             Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits]("https://github.com/marc-sturm/ngs-bits")