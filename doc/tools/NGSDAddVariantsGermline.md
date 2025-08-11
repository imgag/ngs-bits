### NGSDAddVariantsGermline tool help
	NGSDAddVariantsGermline (2025_05-79-g6c060cfd)
	
	Imports variants of a processed sample into the NGSD.
	
	Mandatory parameters:
	  -ps <string>      Processed sample name
	
	Optional parameters:
	  -var <file>       Small variant list in GSvar format (as produced by megSAP).
	                    Default value: ''
	  -var_update       Import missing small variants - doesn't change others.
	                    Default value: 'false'
	  -cnv <file>       CNV list in TSV format (as produced by megSAP).
	                    Default value: ''
	  -sv <file>        SV list in BEDPE format (as produced by megSAP).
	                    Default value: ''
	  -re <file>        RE list in VCF format (as produced by megSAP).
	                    Default value: ''
	  -force            Force import of small variants (they are skipped, if the same callset is already in NGSD).
	                    Default value: 'false'
	  -out <file>       Output file. If unset, writes to STDOUT.
	                    Default value: ''
	  -max_af <float>   Maximum allele frequency of small variants to import (gnomAD).
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
	
### NGSDAddVariantsGermline changelog
	NGSDAddVariantsGermline 2025_05-79-g6c060cfd
	
	2025-07-14 Changed behaviour of 'force' parameter (affects only small variants if callset was already imported now).
	2024-08-28 Merged all force parameters into one. Implmented skipping of small variants import if the same callset was already imported.
	2021-07-19 Added support for 'CADD' and 'SpliceAI' columns in 'variant' table.
[back to ngs-bits](https://github.com/imgag/ngs-bits)