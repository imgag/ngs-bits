### RnaQC tool help
	RnaQC (2022_04-120-g0b2ddab9)
	
	Calculates QC metrics for RNA samples.
	
	Mandatory parameters:
	  -bam <file>                Input BAM/CRAM file.
	  -housekeeping_genes <file> BED file containing the exon region of housekeeping genes.
	
	Optional parameters:
	  -out <file>                Output qcML file. If unset, writes to STDOUT.
	                             Default value: ''
	  -splicing <file>           TSV file containing spliced reads by gene.
	                             Default value: ''
	  -expression <file>         TSV file containing RNA expression.
	                             Default value: ''
	  -ref <file>                Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                             Default value: ''
	  -min_mapq <int>            Set minimal mapping quality (default:0)
	                             Default value: '1'
	  -txt                       Writes TXT format instead of qcML.
	                             Default value: 'false'
	
	Special parameters:
	  --help                     Shows this help and exits.
	  --version                  Prints version and exits.
	  --changelog                Prints changeloge and exits.
	  --tdx                      Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### RnaQC changelog
	RnaQC 2022_04-120-g0b2ddab9
	
	2022-05-12 Changed TPM cutoffs.
	2022-04-27 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)