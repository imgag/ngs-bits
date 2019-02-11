### SampleSimilarity tool help
	SampleSimilarity (2018_11-127-gbd77909)
	
	Calculates pairwise sample similarity metrics from VCF/BAM files.
	
	In VCF mode, multi-allelic variants are not supported. Use 'skip_multi' to ignore them, or VcfBreakMulti to split multi-allelic variants into several lines.
	Multi-sample VCFs are not supported. Use VcfExtractSamples to split them to one VCF per sample.
	In VCF mode, it is assumed that variant lists are left-normalized, e.g. with VcfLeftNormalize.
	
	Mandatory parameters:
	  -in <filelist>      Input variant lists in VCF format (two or more). If only one file is given, each line in this file is interpreted as an input file path.
	
	Optional parameters:
	  -out <file>         Output file. If unset, writes to STDOUT.
	                      Default value: ''
	  -mode <enum>        Mode (input format).
	                      Default value: 'vcf'
	                      Valid: 'vcf,bam'
	  -roi <file>         Restrict similarity calculation to variants in target region.
	                      Default value: ''
	  -include_gonosomes  Includes gonosomes into calculation (by default only variants on autosomes are considered).
	                      Default value: 'false'
	  -skip_multi         Skip multi-allelic variants instead of throwing an error (VCF mode).
	                      Default value: 'false'
	  -min_cov <int>      Minimum coverage to consider a SNP for the analysis (BAM mode).
	                      Default value: '30'
	  -max_snps <int>     The maximum number of high-coverage SNPs to extract from BAM. 0 means unlimited (BAM mode).
	                      Default value: '2000'
	  -build <enum>       Genome build used to generate the input (BAM mode).
	                      Default value: 'hg19'
	                      Valid: 'hg19,hg38'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SampleSimilarity changelog
	SampleSimilarity 2018_11-127-gbd77909
	
	2019-02-08 Massive speed-up by caching of variants/genotypes instead of loading them again for each comparison.
	2018-11-26 Add flag 'skip_multi' to ignore multi-allelic sites.
	2018-07-11 Added build switch for hg38 support.
	2018-06-20 Added IBS0 and IBS2 metrics and renamed tool to SampleSimilarity (was SampleCorrelation).
	2018-01-05 Added multi-sample support and VCF input file support.
	2017-07-22 Added 'roi' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)