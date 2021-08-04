### GraphStringDb tool help
	GraphStringDb (2021_06-48-gfc326851)
	
	Creates simple representation of String-DB interaction graph.
	
	Mandatory parameters:
	  -string <file>     String-DB file with protein interactions.
	  -alias <file>      Input TSV file with aliases for String protein IDs.
	  -out <file>        Output TSV file with edges.
	
	Optional parameters:
	  -min_score <float> Minimum confidence score cutoff for String-DB interaction (0-1).
	                     Default value: '0.4'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### GraphStringDb changelog
	GraphStringDb 2021_06-48-gfc326851
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)