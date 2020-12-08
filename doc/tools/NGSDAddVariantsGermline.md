### NGSDAddVariantsGermline tool help
	NGSDAddVariantsGermline (2020_03-184-g27235379)
	
	Imports variants of a processed sample into the NGSD.
	
	Mandatory parameters:
	  -ps <string>    Processed sample name
	
	Optional parameters:
	  -var <file>     Small variant list in GSvar format (as produced by megSAP).
	                  Default value: ''
	  -var_force      Force import of small variants, even if already imported.
	                  Default value: 'false'
	  -cnv <file>     CNV list in TSV format (as produced by megSAP).
	                  Default value: ''
	  -cnv_force      Force import of CNVs, even if already imported.
	                  Default value: 'false'
	  -sv <file>      SV list in BEDPE format (as produced by megSAP).
	                  Default value: ''
	  -sv_force       Force import of SVs, even if already imported.
	                  Default value: 'false'
	  -out <file>     Output file. If unset, writes to STDOUT.
	                  Default value: ''
	  -max_af <float> Maximum allele frequency of small variants to import (1000g and gnomAD).
	                  Default value: '0.05'
	  -test           Uses the test database instead of on the production database.
	                  Default value: 'false'
	  -debug          Enable verbose debug output.
	                  Default value: 'false'
	  -no_time        Disable timing output.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDAddVariantsGermline changelog
	NGSDAddVariantsGermline 2020_03-184-g27235379
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)