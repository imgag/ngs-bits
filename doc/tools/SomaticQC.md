### SomaticQC tool help
	SomaticQC (2021_12-170-g1acf8802)
	
	Calculates QC metrics based on tumor-normal pairs.
	
	SomaticQC integrates the output of the other QC tools and adds several metrics specific for tumor-normal pairs.
	All tools produce qcML, a generic XML format for QC of -omics experiments, which we adapted for NGS.
	
	Mandatory parameters:
	  -tumor_bam <file>    Input tumor BAM/CRAM file.
	  -normal_bam <file>   Input normal BAM/CRAM file.
	  -somatic_vcf <file>  Input somatic VCF file.
	
	Optional parameters:
	  -out <file>          Output qcML file. If unset, writes to STDOUT.
	                       Default value: ''
	  -links <filelist>    Files that appear in the link part of the qcML file.
	                       Default value: ''
	  -target_bed <file>   Target file used for tumor and normal experiment.
	                       Default value: ''
	  -target_exons <file> BED file containing target exons, neccessary for TMB calculation. Please provide a file that contains the coordinates of all exons in the reference genome.
	                       Default value: ''
	  -blacklist <file>    BED file containing regions which are ignored in TMB calculation.
	                       Default value: ''
	  -tsg_bed <file>      BED file containing regions of tumor suppressor genes for TMB calculation.
	                       Default value: ''
	  -ref <file>          Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                       Default value: ''
	  -skip_plots          Skip plots (intended to increase speed of automated tests).
	                       Default value: 'false'
	  -build <enum>        Genome build used to generate the input.
	                       Default value: 'hg38'
	                       Valid: 'hg19,hg38'
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SomaticQC changelog
	SomaticQC 2021_12-170-g1acf8802
	
	2020-11-27 Added CRAM support.
	2018-07-11 Added build switch for hg38 support.
	2017-07-28 Added somatic allele frequency histogram and tumor estimate.
	2017-01-16 Increased speed for mutation profile, removed genome build switch.
	2016-08-25 Version used in the application note.
[back to ngs-bits](https://github.com/imgag/ngs-bits)