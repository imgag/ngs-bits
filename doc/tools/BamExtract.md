### BamExtract tool help
	BamExtract (2023_11-42-ga9d1687d)
	
	Extract reads from BAM/CRAM by read name.
	
	Mandatory parameters:
	  -in <file>   Input BAM/CRAM file.
	  -ids <file>  Input text file containing read names (one per line).
	  -out <file>  Output BAM/CRAM file with matching reads.
	
	Optional parameters:
	  -out2 <file> Output BAM/CRAM file with not matching reads.
	               Default value: ''
	  -ref <file>  Reference genome for CRAM support (mandatory if CRAM is used).
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamExtract changelog
	BamExtract 2023_11-42-ga9d1687d
	
	2023-11-30 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)