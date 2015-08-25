### SampleGender tool help
	SampleGender (0.1-52-g9f9161f)
	
	Determines the gender of a sample from the BAM file.
	
	Mandatory parameters:
	  -in <file>          Input BAM file.
	  -method <enum>      Method selection: Read distribution on X and Y chromosome (xy), or fraction of heterocygous variants on X chromosome (hetx).
	                      Valid: 'xy,hetx'
	
	Optional parameters:
	  -out <file>         Output file. If unset, writes to STDOUT.
	                      Default value: ''
	  -max_female <float> Maximum Y/X ratio for female (method xy).
	                      Default value: '0.059999999999999998'
	  -min_male <float>   Minimum Y/X ratio for male (method xy).
	                      Default value: '0.089999999999999997'
	  -min_female <float> Minimum heterocygous SNP fraction for female (method hetx).
	                      Default value: '0.23999999999999999'
	  -max_male <float>   Maximum heterocygous SNP fraction for male (method hetx).
	                      Default value: '0.14999999999999999'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --tdx               Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)