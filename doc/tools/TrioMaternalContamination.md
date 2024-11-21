### TrioMaternalContamination tool help
	TrioMaternalContamination (2024_08-110-g317f43b9)
	
	Detects maternal contamination of a child using SNPs from parents.
	
	Determines the percentage of heterozygous SNPs passed on to the child from mother/father.
	This percentage should be similar for mother/father. If it is not, maternal contamination is likely.
	
	Mandatory parameters:
	  -bam_m <file>        Input BAM/CRAM file of mother.
	  -bam_f <file>        Input BAM/CRAM file of father.
	  -bam_c <file>        Input BAM/CRAM file of child.
	
	Optional parameters:
	  -min_depth <int>     Minimum depth for calling SNPs.
	                       Default value: '3'
	  -min_alt_count <int> Minimum number of alternate observations for calling a SNP.
	                       Default value: '1'
	  -build <enum>        Genome build used to generate the input.
	                       Default value: 'hg38'
	                       Valid: 'hg19,hg38'
	  -out <file>          Output file. If unset, writes to STDOUT.
	                       Default value: ''
	  -ref <file>          Reference genome for CRAM support (mandatory if CRAM is used).
	                       Default value: ''
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]    Settings override file (no other settings files are used).
	
### TrioMaternalContamination changelog
	TrioMaternalContamination 2024_08-110-g317f43b9
	
	2020-11-27 Added CRAM support.
	2020-06-18 Initial version of the tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)