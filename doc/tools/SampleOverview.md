### SampleOverview tool help
	SampleOverview (0.1-578-g311d9bf)
	
	Creates a variant overview table from several samples.
	
	Mandatory parameters:
	  -in <filelist>     Input variant lists in TSV format.
	  -out <file>        Output variant list file in TSV format.
	
	Optional parameters:
	  -window <int>      Window to consider around indel positions to compensate for differing alignments.
	                     Default value: '100'
	  -add_cols <string> Comma-separated list of input columns that shall be added to the output. By default, all columns that are present in all input files.
	                     Default value: '[auto]'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SampleOverview changelog
	SampleOverview 0.1-578-g311d9bf
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)