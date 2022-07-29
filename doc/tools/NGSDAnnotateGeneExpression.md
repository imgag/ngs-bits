### NGSDAnnotateGeneExpression tool help
	NGSDAnnotateGeneExpression (2022_07-37-g22d4e20c)
	
	Annotates a GSvar file with RNA expression data.
	
	Mandatory parameters:
	  -in <file>              Input GSvar file of DNA sample.
	  -out <file>             Output GSvar file.
	  -rna_ps <string>        Processed sample name of the associated .
	
	Optional parameters:
	  -cohort_strategy <enum> Determines which samples are used as reference cohort.
	                          Default value: 'RNA_COHORT_GERMLINE'
	                          Valid: 'RNA_COHORT_GERMLINE,RNA_COHORT_GERMLINE_PROJECT,RNA_COHORT_SOMATIC'
	  -test                   Uses the test database instead of on the production database.
	                          Default value: 'false'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDAnnotateGeneExpression changelog
	NGSDAnnotateGeneExpression 2022_07-37-g22d4e20c
	
	2022-06-14 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)