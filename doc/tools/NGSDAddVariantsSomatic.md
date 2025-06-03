### NGSDAddVariantsSomatic tool help
	NGSDAddVariantsSomatic (2025_03-80-g74f31dd7)
	
	Imports variants of a tumor-normal processed sample into the NGSD.
	
	Mandatory parameters:
	  -t_ps <string>    Tumor processed sample name
	
	Optional parameters:
	  -n_ps <string>    Normal processed sample name
	                    Default value: ''
	  -var <file>       Small variant list in GSvar format (as produced by megSAP).
	                    Default value: ''
	  -cnv <file>       CNV list in TSV format (as produced by megSAP).
	                    Default value: ''
	  -sv <file>        SV list in BEDPE format (as produced by megSAP).
	                    Default value: ''
	  -force            Force import of variants, even if already imported.
	                    Default value: 'false'
	  -out <file>       Output file. If unset, writes to STDOUT.
	                    Default value: ''
	  -max_af <float>   Maximum allele frequency of small variants to import (gnomAD) for import of tumor-only.
	                    Default value: '0.05'
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	  -debug            Enable verbose debug output.
	                    Default value: 'false'
	  -no_time          Disable timing output.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDAddVariantsSomatic changelog
	NGSDAddVariantsSomatic 2025_03-80-g74f31dd7
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)