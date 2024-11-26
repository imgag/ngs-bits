### GraphStringDb tool help
	GraphStringDb (2024_08-110-g317f43b9)
	
	Creates simple representation of String-DB interaction graph.
	
	Mandatory parameters:
	  -string <file>     String-DB file with protein interactions (https://stringdb-static.org/download/protein.links.v11.5/9606.protein.links.v11.5.txt.gz).
	  -alias <file>      Input TSV file with aliases for String protein IDs (https://stringdb-static.org/download/protein.aliases.v11.5/9606.protein.aliases.v11.5.txt.gz).
	  -out <file>        Output TSV file with edges.
	
	Optional parameters:
	  -min_score <float> Minimum confidence score cutoff for String-DB interaction (0-1).
	                     Default value: '0.4'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]  Settings override file (no other settings files are used).
	
### GraphStringDb changelog
	GraphStringDb 2024_08-110-g317f43b9
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)