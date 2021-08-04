### GenePrioritization tool help
	GenePrioritization (2021_06-48-gfc326851)
	
	Performs gene prioritization based on list of known disease genes and a PPI graph.
	
	Mandatory parameters:
	  -in <file>       Input TSV file with one gene identifier per line.
	  -graph <file>    Graph TSV file with two gene identifiers per line.
	  -out <file>      Output TSV file containing prioritized genes.
	
	Optional parameters:
	  -method <enum>   Gene prioritization method to use.
	                   Default value: 'flooding'
	                   Valid: 'flooding,random_walk'
	  -n <int>         Number of network diffusion iterations (flooding).
	                   Default value: '2'
	  -restart <float> Restart probability (random_walk).
	                   Default value: '0.4'
	  -debug <file>    Output TSV file for debugging
	                   Default value: ''
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --changelog      Prints changeloge and exits.
	  --tdx            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### GenePrioritization changelog
	GenePrioritization 2021_06-48-gfc326851
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)