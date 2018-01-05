### SampleCorrelation tool help
	SampleCorrelation (0.1-970-gf28ad3f)
	
	Calculates the variant overlap and correlation of two variant lists.
	
	Mandatory parameters:
	  -in <filelist>  Input variant lists in GSvar format (two or more).
	
	Optional parameters:
	  -out <file>     Output file. If unset, writes to STDOUT.
	                  Default value: ''
	  -mode <enum>    Input file format overwrite.
	                  Default value: 'gsvar'
	                  Valid: 'gsvar,vcf,bam'
	  -window <int>   Window to consider around indel positions to compensate for differing alignments (GSvar mode).
	                  Default value: '100'
	  -min_cov <int>  Minimum coverage to consider a SNP for the analysis (BAM mode).
	                  Default value: '30'
	  -max_snps <int> The maximum number of high-coverage SNPs to analyze. 0 means unlimited (BAM mode).
	                  Default value: '500'
	  -roi <file>     Target region used to speed up calculations e.g. for panel data (BAM mode).
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SampleCorrelation changelog
	SampleCorrelation 0.1-970-gf28ad3f
	
	2018-01-05 Added multi-sample support and VCF input file support.
	2017-07-22 Added 'roi' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)