### GenePrioritization tool help
	GenePrioritization (2024_08-113-g94a3b440)
	
	Performs gene prioritization based on list of known disease genes of a disease and a PPI graph.
	
	Mandatory parameters:
	  -in <file>        Input TSV file with one gene identifier per line (known disease genes of a disease).
	  -graph <file>     Graph TSV file with two gene identifiers per line (PPI graph).
	  -out <file>       Output TSV file containing prioritized genes for the disease.
	
	Optional parameters:
	  -method <enum>    Gene prioritization method to use.
	                    Default value: 'flooding'
	                    Valid: 'flooding,random_walk'
	  -n <int>          Number of network diffusion iterations (flooding).
	                    Default value: '2'
	  -restart <float>  Restart probability (random_walk).
	                    Default value: '0.4'
	  -debug <file>     Output TSV file for debugging
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### GenePrioritization changelog
	GenePrioritization 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)