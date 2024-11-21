### NGSDAnnotateRNA tool help
	NGSDAnnotateRNA (2024_08-110-g317f43b9)
	
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
	  -hpa_file <file>        TSV file containing the Human Protein Atlas (https://www.proteinatlas.org) to annotate gene expression
	                          Default value: ''
	  -update_genes           Update annotated gene names with approved gene names from the NGSD
	                          Default value: 'false'
	  -test                   Uses the test database instead of on the production database.
	                          Default value: 'false'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]       Settings override file (no other settings files are used).
	
### NGSDAnnotateRNA changelog
	NGSDAnnotateRNA 2024_08-110-g317f43b9
	
	2022-09-15 Added annotation of transcript ids.
	2022-08-18 Added ability to update gene names.
	2022-08-11 Added HPA annotation support.
	2022-07-13 Added support for exons.
	2022-06-09 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)