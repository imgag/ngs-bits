### BedAnnotateGenes tool help
	BedAnnotateGenes (0.1-893-g1246b60)
	
	Annotates BED file regions with gene names.
	
	Optional parameters:
	  -in <file>    Input BED file. If unset, reads from STDIN.
	                Default value: ''
	  -out <file>   Output BED file. If unset, writes to STDOUT.
	                Default value: ''
	  -extend <int> The number of bases to extend the gene regions before annotation.
	                Default value: '0'
	  -test         Uses the test database instead of on the production database.
	                Default value: 'false'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --changelog   Prints changeloge and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedAnnotateGenes changelog
	BedAnnotateGenes 0.1-893-g1246b60
	
	2017-11-03 Now appends a column to the BED file instead of always writing it into the 4th column.
[back to ngs-bits](https://github.com/imgag/ngs-bits)