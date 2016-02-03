### SampleDiff tool help
	SampleDiff (0.1-190-g94e4c3d)
	
	Calculates the differences/overlap between variant lists.
	
	Mandatory parameters:
	  -in1 <file>   Input variant list in TSV format.
	  -in2 <file>   Input variant list in TSV format.
	
	Optional parameters:
	  -out <file>   Output file. If unset, writes to STDOUT.
	                Default value: ''
	  -window <int> Window to consider around indel positions to compensate for differing alignments.
	                Default value: '100'
	  -ei           Exact indel matches only. If unset, all indels in the window are considered matches.
	                Default value: 'false'
	  -sm           Also show matches. If unset, matching variants are not printed.
	                Default value: 'false'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)