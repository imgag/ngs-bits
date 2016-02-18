### BedGeneOverlap tool help
	BedGeneOverlap (0.1-190-g94e4c3d)
	
	Calculates how much of each overlapping gene is covered.
	
	Mandatory parameters:
	  -source <enum> Transcript source database.
	                 Valid: 'ccds,ucsc'
	
	Optional parameters:
	  -in <file>     Input BED file. If unset, reads from STDIN.
	                 Default value: ''
	  -out <file>    Output TSV file. If unset, writes to STDOUT.
	                 Default value: ''
	  -test          Uses the test database instead of on the production database.
	                 Default value: 'false'
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)