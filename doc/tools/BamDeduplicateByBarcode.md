### BamDeduplicateByBarcode tool help
	BamDeduplicateByBarcode (0.1-645-gfc628cd)
	
	Removes duplicates from a bam file based on a molecular barcode file.
	
	Mandatory parameters:
	  -bam <file>           Input BAM file.
	  -index <file>         FASTQ file containing the molecular barcode sequences.
	  -out <file>           Output BAM file.
	
	Optional parameters:
	  -test                 adjust output for testing purposes
	                        Default value: 'false'
	  -del_amb              remove barcode families where multiple read are tied for most frequent count
	                        Default value: 'false'
	  -min_group <int>      minimal numbers of reads to keep a barcode group.
	                        Default value: '1'
	  -dist <int>           edit distance for single read matching .
	                        Default value: '0'
	  -mip_file <file>      input file for MIPS (reads are filtered and cut to match only MIP inserts).
	                        Default value: ''
	  -hs_file <file>       Agilent's Haloplex HS _amplicons.bed file.
	                        Default value: ''
	  -stats <file>         Output TSV file for statistics).
	                        Default value: ''
	  -nomatch_out <file>   Output Bed file for reads not matching any amplicon).
	                        Default value: ''
	  -duplicate_out <file> Output Bed file for reads removed as duplicates).
	                        Default value: ''
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamDeduplicateByBarcode changelog
	BamDeduplicateByBarcode 0.1-645-gfc628cd
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)