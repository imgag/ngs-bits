### NGSDExtractRNACohort tool help
	NGSDExtractRNACohort (2022_07-52-g9fafa140)
	
	Creates a table with gene expression values for a given set of genes and cohort
	
	Mandatory parameters:
	  -ps <string>              Processed sample name on which the cohort is calculated.
	  -genes <file>             Text file containing gene names which should be included in the table. (1 gene per line.)
	
	Optional parameters:
	  -sample_expression <file> TSV file containing gene expression for processed sample (required if processed sample data hasn't been imported to the database yet)
	                            Default value: ''
	  -out <file>               Output TSV file. If unset, writes to STDOUT.
	                            Default value: ''
	  -cohort_strategy <enum>   Determines which samples are used as reference cohort.
	                            Default value: 'RNA_COHORT_GERMLINE'
	                            Valid: 'RNA_COHORT_GERMLINE,RNA_COHORT_GERMLINE_PROJECT,RNA_COHORT_SOMATIC'
	  -test                     Uses the test database instead of on the production database.
	                            Default value: 'false'
	
	Special parameters:
	  --help                    Shows this help and exits.
	  --version                 Prints version and exits.
	  --changelog               Prints changeloge and exits.
	  --tdx                     Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDExtractRNACohort changelog
	NGSDExtractRNACohort 2022_07-52-g9fafa140
	
	2022-07-21 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)