### CfDnaQC tool help
	CfDnaQC (2021_12-80-g5ff26d8f)
	
	Calculates QC metrics for cfDNA samples.
	
	Mandatory parameters:
	  -bam <file>              Input BAM/CRAM file.
	  -cfdna_panel <file>      Input BED file containing the (personalized) cfDNA panel.
	
	Optional parameters:
	  -out <file>              Output qcML file. If unset, writes to STDOUT.
	                           Default value: ''
	  -tumor_bam <file>        Input tumor BAM/CRAM file for sample similarity.
	                           Default value: ''
	  -related_bams <filelist> BAM files of related cfDNA samples to compute sample similarity.
	                           Default value: ''
	  -error_rates <file>      Input TSV containing umiVar error rates.
	                           Default value: ''
	  -build <enum>            Genome build used to generate the input.
	                           Default value: 'hg38'
	                           Valid: 'hg19,hg38'
	  -ref <file>              Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                           Default value: ''
	  -ref_cram <string>       Reference genome for CRAM support (mandatory if CRAM is used). If set, it is used for tumor and normal file.
	                           Default value: ''
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### CfDnaQC changelog
	CfDnaQC 2021_12-80-g5ff26d8f
	
	2021-12-03 Added correllation between cfDNA samples.
	2021-10-22 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)