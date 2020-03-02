### VariantAnnotateFrequency tool help
	VariantAnnotateFrequency (2019_11-129-g4a5e7bf9)
	
	Annotates a variant list with variant frequencies from a BAM file.
	
	Mandatory parameters:
	  -in <file>     Input variant list to annotate in VCF or GSvar format.
	  -bam <file>    Input BAM file.
	  -out <file>    Output variant list file name (VCF or GSvar).
	
	Optional parameters:
	  -depth         Annotate an additional column containing the depth.
	                 Default value: 'false'
	  -mapq0         Annotate an additional column containing the percentage of mapq 0 reads.
	                 Default value: 'false'
	  -name <string> Column header prefix in output file.
	                 Default value: ''
	  -ref <file>    Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                 Default value: ''
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantAnnotateFrequency changelog
	VariantAnnotateFrequency 2019_11-129-g4a5e7bf9
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)