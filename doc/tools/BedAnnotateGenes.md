### BedAnnotateGenes tool help
	BedAnnotateGenes (2024_08-113-g94a3b440)
	
	Annotates BED file regions with gene names.
	
	Optional parameters:
	  -in <file>        Input BED file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output BED file. If unset, writes to STDOUT.
	                    Default value: ''
	  -extend <int>     The number of bases to extend the gene regions before annotation.
	                    Default value: '0'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	  -clear            Clear all annotations present in the input file.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedAnnotateGenes changelog
	BedAnnotateGenes 2024_08-113-g94a3b440
	
	2017-11-28 Added 'clear' flag.
	2017-11-03 Now appends a column to the BED file instead of always writing it into the 4th column.
[back to ngs-bits](https://github.com/imgag/ngs-bits)