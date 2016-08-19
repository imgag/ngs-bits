### SomaticQC tool help
	SomaticQC (0.1-438-g79b1e8b)
	
	Calculates QC metrics based on tumor-normal pairs.
	
	Mandatory parameters:
	  -tumor_bam <file>    Input tumor bam file.
	  -normal_bam <file>   Input normal bam file.
	  -somatic_vcf <file>  Input somatic vcf file.
	
	Optional parameters:
	  -out <file>          Output qcML file. If unset, writes to STDOUT.
	                       Default value: ''
	  -target_bed <file>   Target file used for tumor and normal experiment.
	                       Default value: ''
	  -links <filelist>    Files that appear in the link part of the qcML file.
	                       Default value: ''
	  -genome_build <enum> Genome build. Option count uses the refence genome taken from the settings file and counts all trippletts on the fly.
	                       Default value: 'hg19'
	                       Valid: 'hg19,hg38,count'
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SomaticQC changelog
	SomaticQC 0.1-438-g79b1e8b
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)