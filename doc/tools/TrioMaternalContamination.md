### TrioMaternalContamination tool help
	TrioMaternalContamination (2020_03-164-gc1e7ad2f)
	
	Detects maternal contamination in a child employing shallow parental sequencing data.
	
	Mandatory parameters:
	  -bam_m <file>        Input BAM file of mother.
	  -bam_f <file>        Input BAM file of father.
	  -bam_c <file>        Input BAM file of child.
	
	Optional parameters:
	  -min_depth <int>     Minimum depth to consider a base.
	                       Default value: '3'
	  -min_alt_count <int> Minimum number of alterations to call a variant.
	                       Default value: '1'
	  -build <enum>        Genome build used to generate the input.
	                       Default value: 'hg19'
	                       Valid: 'hg19,hg38'
	  -out <file>          Output file. If unset, writes to STDOUT.
	                       Default value: ''
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### TrioMaternalContamination changelog
	TrioMaternalContamination 2020_03-164-gc1e7ad2f
	
	2020-05-13 Initial version of the tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)