### SampleGender tool help
	SampleGender (0.1-190-g94e4c3d)
	
	Determines the gender of a sample from the BAM file.
	
	Mandatory parameters:
	  -in <file>          Input BAM file.
	  -method <enum>      Method selection: Read distribution on X and Y chromosome (xy), fraction of heterocygous variants on X chromosome (hetx), or coverage of SRY gene (sry).
	                      Valid: 'xy,hetx,sry'
	
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
	  -sry_cov <float>    Maximum average coverage of SRY gene for males (method sry).
	                      Default value: '20'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)