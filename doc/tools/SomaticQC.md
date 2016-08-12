### SomaticQC tool help
	SomaticQC (0.1-438-g79b1e8b)
	
	Calculates QC metrics based on tumor-normal pairs.
	
	Mandatory parameters:
	  -tumor_bam <file>   Input tumor bam file.
	  -normal_bam <file>  Input normal bam file.
	  -somatic_vcf <file> Input somatic vcf file.
	
	Optional parameters:
	  -count              Count and print tripletts for current reference genome. No qcML file will generated.
	                      Default value: 'false'
	  -target_bed <file>  Target file used for tumor and normal experiment.
	                      Default value: ''
	  -out <file>         Output qcML file. If unset, writes to STDOUT.
	                      Default value: ''
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SomaticQC changelog
	SomaticQC 0.1-438-g79b1e8b
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)