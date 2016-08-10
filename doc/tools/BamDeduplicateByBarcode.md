### BamDeduplicateByBarcode tool help
	BamDeduplicateByBarcode (0.1-420-g3536bb0)
	
	Removes duplicates from a bam file based on a molecular barcode file.
	
	Mandatory parameters:
	  -bam <file>           Input BAM file.
	  -index <file>         Index FASTQ file.
	  -out <file>           Output BAM file.
	
	Optional parameters:
	  -flag                 flag duplicate reads insteadt of deleting them
	                        Default value: 'false'
	  -test                 adjust output for testing purposes
	                        Default value: 'false'
	  -min_group <int>      minimal numbers of reads to keep a barcode group.
	                        Default value: '1'
	  -dist <int>           edit distance for single read matching .
	                        Default value: '0'
	  -mip_file <file>      input file for MIPS (reads are filtered and cut to match only MIP inserts).
	                        Default value: ''
	  -hs_file <file>       input file for Haloplex HS amplicons (reads are filtered to match only amplicons).
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
	BamDeduplicateByBarcode 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)