### SomaticQC tool help
	SomaticQC (0.1-773-g0c7504d)
	
	Calculates QC metrics based on tumor-normal pairs.
	
	SomaticQC integrates the output of the other QC tools and adds several metrics specific for tumor-normal pairs. The genome build form the settings file will be used during calcuation of QC metrics. All tools produce qcML, a generic XML format for QC of -omics experiments, which we adapted for NGS.
	
	Mandatory parameters:
	  -tumor_bam <file>   Input tumor bam file.
	  -normal_bam <file>  Input normal bam file.
	  -somatic_vcf <file> Input somatic vcf file.
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
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SomaticQC changelog
	SomaticQC 0.1-773-g0c7504d
	
	2017-01-16 Increased speed for mutation profile, removed genome build switch.
	2016-08-25 Version used in the application note.
[back to ngs-bits](https://github.com/imgag/ngs-bits)