### NGSDAnnotateRNA tool help
	NGSDAnnotateRNA (2022_07-37-g22d4e20c)
	
	Annotates a RNA expression TSV file with cohort information.
	
	Mandatory parameters:
	  -ps <string>            Processed sample name of the input file.
	
	Optional parameters:
	  -in <file>              Input TSV file. If unset, reads from STDIN.
	                          Default value: ''
	  -out <file>             Output TSV file. If unset, writes to STDOUT.
	                          Default value: ''
	  -mode <enum>            Determines if genes or exons should be annotated.
	                          Default value: 'genes'
	                          Valid: 'genes,exons'
	  -cohort_strategy <enum> Determines which samples are used as reference cohort.
	                          Default value: 'RNA_COHORT_GERMLINE'
	                          Valid: 'RNA_COHORT_GERMLINE,RNA_COHORT_GERMLINE_PROJECT,RNA_COHORT_SOMATIC'
	  -corr <file>            File path to output file containing the spearman correlation to cohort mean.
	                          Default value: ''
	  -test                   Uses the test database instead of on the production database.
	                          Default value: 'false'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDAnnotateRNA changelog
	NGSDAnnotateRNA 2022_07-37-g22d4e20c
	
	2022-07-13 Added support for exons.
	2022-06-09 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)