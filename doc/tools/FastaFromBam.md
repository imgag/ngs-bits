### FastaFromBam tool help
	FastaFromBam (2024_08-36-g4fed1f49)
	
	Download the reference genome FASTA file for a BAM/CRAM file.
	
	Mandatory parameters:
	  -in <file>   Input BAM/CRAM file.
	  -out <file>  Output reference genome FASTA file.
	
	Optional parameters:
	  -ref <file>  Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastaFromBam changelog
	FastaFromBam 2024_08-36-g4fed1f49
	
	2024-08-18 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)