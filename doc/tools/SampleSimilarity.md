### SampleSimilarity tool help
	SampleSimilarity (2018_11-7-g60f117b)
	
	Calculates pairwise sample similarity metrics from VCF/BAM files.
	
	In VCF mode, multi-allelic variants are not supported. Use VcfBreakMulti to split multi-allelic variants into several lines.
	Multi-sample VCFs are not accepted as input. Use VcfExtractSamples to split them to one VCF per sample.
	
	Mandatory parameters:
	  -in <filelist>      Input variant lists in VCF format (two or more).
	
	Optional parameters:
	  -out <file>         Output file. If unset, writes to STDOUT.
	                      Default value: ''
	  -mode <enum>        Input file format overwrite.
	                      Default value: 'vcf'
	                      Valid: 'vcf,bam'
	  -include_gonosomes  Includes gonosomes into calculation (by default only variants on autosomes are considered).
	                      Default value: 'false'
	  -skip_multi         Skip multi-allelic variants instead of throwing an error (VCF mode).
	                      Default value: 'false'
	  -window <int>       Window to consider around indel positions to compensate for differing alignments (VCF mode).
	                      Default value: '100'
	  -min_cov <int>      Minimum coverage to consider a SNP for the analysis (BAM mode).
	                      Default value: '30'
	  -max_snps <int>     The maximum number of high-coverage SNPs to analyze. 0 means unlimited (BAM mode).
	                      Default value: '500'
	  -roi <file>         Target region used to speed up calculations e.g. for panel data (BAM mode).
	                      Default value: ''
	  -build <enum>       Genome build used to generate the input (BAM mode).
	                      Default value: 'hg19'
	                      Valid: 'hg19,hg38'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SampleSimilarity changelog
	SampleSimilarity 2018_11-7-g60f117b
	
	2018-11-26 Add flag 'skip_multi' to ignore multi-allelic sites.
	2018-07-11 Added build switch for hg38 support.
	2018-06-20 Added IBS0 and IBS2 metrics and renamed tool to SampleSimilarity (was SampleCorrelation).
	2018-01-05 Added multi-sample support and VCF input file support.
	2017-07-22 Added 'roi' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)