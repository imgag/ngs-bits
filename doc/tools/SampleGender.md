### SampleGender tool help
	SampleGender (2018_11-55-g768b41c)
	
	Determines the gender of a sample from the BAM file.
	
	Mandatory parameters:
	  -in <filelist>      Input BAM file(s).
	  -method <enum>      Method selection: Read distribution on X and Y chromosome (xy), fraction of heterocygous variants on X chromosome (hetx), or coverage of SRY gene (sry).
	                      Valid: 'xy,hetx,sry'
	
	Optional parameters:
	  -out <file>         Output TSV file - one line per input BAM file. If unset, writes to STDOUT.
	                      Default value: ''
	  -max_female <float> Maximum Y/X ratio for female (method xy).
	                      Default value: '0.06'
	  -min_male <float>   Minimum Y/X ratio for male (method xy).
	                      Default value: '0.09'
	  -min_female <float> Minimum heterocygous SNP fraction for female (method hetx).
	                      Default value: '0.24'
	  -max_male <float>   Maximum heterocygous SNP fraction for male (method hetx).
	                      Default value: '0.15'
	  -sry_cov <float>    Minimum average coverage of SRY gene for males (method sry).
	                      Default value: '20'
	  -build <enum>       Genome build used to generate the input (methods hetx and sry).
	                      Default value: 'hg19'
	                      Valid: 'hg19,hg38'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SampleGender changelog
	SampleGender 2018_11-55-g768b41c
	
	2018-07-13 Change of output to TSV format for batch support.
	2018-07-11 Added build switch for hg38 support.
[back to ngs-bits](https://github.com/imgag/ngs-bits)