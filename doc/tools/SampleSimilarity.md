### SampleSimilarity tool help
	SampleSimilarity (2025_03-80-g74f31dd7)
	
	Calculates pairwise sample similarity metrics from VCF/BAM/CRAM files.
	
	In VCF mode, multi-allelic variants are not supported and skipped. Use VcfBreakMulti to split multi-allelic variants into several lines if you want to use them.
	Multi-sample VCFs are not supported. Use VcfExtractSamples to split them to one VCF per sample.
	In VCF mode, it is assumed that variant lists are left-normalized, e.g. with VcfLeftNormalize.
	BAM mode supports BAM as well as CRAM files.
	Note: When working on hg38 WES or WGS samples, it is recommended to use the 'roi_hg38_wes_wgs' flag!
	
	Mandatory parameters:
	  -in <filelist>      Input variant lists in VCF format (two or more). If only one file is given, each line in this file is interpreted as an input file path.
	
	Optional parameters:
	  -out <file>         Output file. If unset, writes to STDOUT.
	                      Default value: ''
	  -mode <enum>        Mode (input format).
	                      Default value: 'vcf'
	                      Valid: 'vcf,gsvar,bam'
	  -roi <file>         Restrict similarity calculation to variants in target region.
	                      Default value: ''
	  -roi_hg38_wes_wgs   Used pre-defined high-confidence coding region of hg38. Speeds up calculations, especially for WGS. Also makes scores comparable when mixing WES and WGS or different WES kits.
	                      Default value: 'false'
	  -include_gonosomes  Includes gonosomes into calculation (by default only variants on autosomes are considered).
	                      Default value: 'false'
	  -min_cov <int>      Minimum coverage to consider a SNP for the analysis (BAM mode).
	                      Default value: '30'
	  -max_snps <int>     The maximum number of high-coverage SNPs to extract from BAM/CRAM. 0 means unlimited (BAM mode).
	                      Default value: '5000'
	  -build <enum>       Genome build used to generate the input (BAM mode).
	                      Default value: 'hg38'
	                      Valid: 'hg19,hg38'
	  -ref <file>         Reference genome for CRAM support (mandatory if CRAM is used).
	                      Default value: ''
	  -long_read          Support long reads (BAM mode).
	                      Default value: 'false'
	  -debug              Print debug output.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]   Settings override file (no other settings files are used).
	
### SampleSimilarity changelog
	SampleSimilarity 2025_03-80-g74f31dd7
	
	2023-12-22 Added 'roi_hg38_wes_wgs' flag.
	2022-07-07 Changed BAM mode: max_snps is now 5000 by default because this results in a better separation of related and unrelated samples.
	2022-06-30 Changed GSvar mode: MODIFIER impact variants are now ingnored to make scores more similar between exomes and genomes.
	2020-11-27 Added CRAM support.
	2019-02-08 Massive speed-up by caching of variants/genotypes instead of loading them again for each comparison.
	2018-07-11 Added build switch for hg38 support.
	2018-06-20 Added IBS0 and IBS2 metrics and renamed tool to SampleSimilarity (was SampleCorrelation).
	2018-01-05 Added multi-sample support and VCF input file support.
	2017-07-22 Added 'roi' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)