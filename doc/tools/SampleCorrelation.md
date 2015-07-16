### SampleCorrelation tool help
	SampleCorrelation (0.1-46-gb124721)
	
	Calculates the variant overlap and correlation of two variant lists.
	
	Mandatory parameters:
	  -in1 <file>     Input variant list in TSV format.
	  -in2 <file>     Input variant list in TSV format.
	
	Optional parameters:
	  -out <file>     Output file. If unset, writes to STDOUT.
	                  Default value: ''
	  -bam            Input files are BAM files instead of TSV files.
	                  Default value: 'false'
	  -window <int>   Window to consider around indel positions to compensate for differing alignments (TSV input).
	                  Default value: '100'
	  -min_cov <int>  Minimum coverage to consider a SNP for the analysis. (BAM input)
	                  Default value: '30'
	  -max_snps <int> The maximum number of high-coverage SNPs to analyze. 0 means unlimited. (BAM input)
	                  Default value: '500'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --tdx           Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)