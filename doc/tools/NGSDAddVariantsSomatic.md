### NGSDAddVariantsSomatic tool help
	NGSDAddVariantsSomatic (2024_02-126-g4f44d5e5)
	
	Imports variants of a tumor-normal processed sample into the NGSD.
	
	Mandatory parameters:
	  -t_ps <string> Tumor processed sample name
	  -n_ps <string> Normal processed sample name
	
	Optional parameters:
	  -var <file>    Small variant list (i.e. SNVs and small INDELs) in GSvar format (as produced by megSAP).
	                 Default value: ''
	  -var_force     Force import of detected small variants, even if already imported.
	                 Default value: 'false'
	  -cnv <file>    CNV list in TSV format (as produced by megSAP).
	                 Default value: ''
	  -cnv_force     Force import of CNVs, even if already imported.
	                 Default value: 'false'
	  -sv <file>     SV list in TSV format (as produced by megSAP).
	                 Default value: ''
	  -sv_force      Force import of SVs, even if already imported.
	                 Default value: 'false'
	  -out <file>    Output file. If unset, writes to STDOUT.
	                 Default value: ''
	  -test          Uses the test database instead of on the production database.
	                 Default value: 'false'
	  -debug         Enable verbose debug output.
	                 Default value: 'false'
	  -no_time       Disable timing output.
	                 Default value: 'false'
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDAddVariantsSomatic changelog
	NGSDAddVariantsSomatic 2024_02-126-g4f44d5e5
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)