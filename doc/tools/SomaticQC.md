### SomaticQC tool help
	SomaticQC (0.1-487-g4a3dc62)
	
	Calculates QC metrics based on tumor-normal pairs.
	
	SomaticQC integrates the output of the other QC tools and adds several metrics specific for tumor-normal pairs. All tools produce qcML, a generic XML format for QC of -omics experiments, which we adapted for NGS.
	
	Mandatory parameters:
	  -tumor_bam <file>    Input tumor bam file.
	  -normal_bam <file>   Input normal bam file.
	  -somatic_vcf <file>  Input somatic vcf file.
	
	Optional parameters:
	  -out <file>          Output qcML file. If unset, writes to STDOUT.
	                       Default value: ''
	  -links <filelist>    Files that appear in the link part of the qcML file.
	                       Default value: ''
	  -target_bed <file>   Target file used for tumor and normal experiment. The genome build form the settings file will be used to count variant signature.
	                       Default value: ''
	  -genome_build <enum> For whole genome sequencing precalculated triplet counts are available for some genome builds. Selecting one of the available options will use these counts. Otherwise the triplets will be counted on the fly. This parameter is not required if a target_bed file is given.
	                       Default value: 'count'
	                       Valid: 'hg19,hg38,count'
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SomaticQC changelog
	SomaticQC 0.1-487-g4a3dc62
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)