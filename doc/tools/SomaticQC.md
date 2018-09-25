### SomaticQC tool help
	SomaticQC (2018_06-35-ga1d5548)
	
	Calculates QC metrics based on tumor-normal pairs.
	
	SomaticQC integrates the output of the other QC tools and adds several metrics specific for tumor-normal pairs.
	All tools produce qcML, a generic XML format for QC of -omics experiments, which we adapted for NGS.
	
	Mandatory parameters:
	  -tumor_bam <file>   Input tumor BAM file.
	  -normal_bam <file>  Input normal BAM file.
	  -somatic_vcf <file> Input somatic VCF file.
	  -out <file>         Output qcML file. If unset, writes to STDOUT.
	
	Optional parameters:
	  -links <filelist>   Files that appear in the link part of the qcML file.
	                      Default value: ''
	  -target_bed <file>  Target file used for tumor and normal experiment.
	                      Default value: ''
	  -ref_fasta <file>   Reference fasta file. If unset the reference file from the settings file will be used.
	                      Default value: ''
	  -skip_plots         Skip plots (intended to increase speed of automated tests).
	                      Default value: 'false'
	  -build <enum>       Genome build used to generate the input.
	                      Default value: 'hg19'
	                      Valid: 'hg19,hg38'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SomaticQC changelog
	SomaticQC 2018_06-35-ga1d5548
	
	2018-07-11 Added build switch for hg38 support.
	2017-07-28 Added somatic allele frequency histogram and tumor estimate.
	2017-01-16 Increased speed for mutation profile, removed genome build switch.
	2016-08-25 Version used in the application note.
[back to ngs-bits](https://github.com/imgag/ngs-bits)