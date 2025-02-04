### RnaQC tool help
	RnaQC (2024_08-110-g317f43b9)
	
	Calculates QC metrics for RNA samples.
	
	Mandatory parameters:
	  -bam <file>                Input BAM/CRAM file.
	
	Optional parameters:
	  -housekeeping_genes <file> BED file containing the exon region of housekeeping genes.
	                             Default value: ''
	  -roi <file>                BED file containing the target region of the analysis.
	                             Default value: ''
	  -out <file>                Output qcML file. If unset, writes to STDOUT.
	                             Default value: ''
	  -splicing <file>           TSV file containing spliced reads by gene.
	                             Default value: ''
	  -expression <file>         TSV file containing RNA expression.
	                             Default value: ''
	  -ref <file>                Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                             Default value: ''
	  -min_mapq <int>            Set minimal mapping quality (default:1)
	                             Default value: '1'
	  -txt                       Writes TXT format instead of qcML.
	                             Default value: 'false'
	
	Special parameters:
	  --help                     Shows this help and exits.
	  --version                  Prints version and exits.
	  --changelog                Prints changeloge and exits.
	  --tdx                      Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]          Settings override file (no other settings files are used).
	
### RnaQC changelog
	RnaQC 2024_08-110-g317f43b9
	
	2023-03-22 Added optional target region.
	2022-07-12 Made housekeeping genes optional.
	2022-05-12 Changed TPM cutoffs.
	2022-04-27 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)