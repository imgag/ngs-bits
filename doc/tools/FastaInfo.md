### FastaInfo tool help
	FastaInfo (2024_08-36-g4fed1f49)
	
	Basic info on a FASTA file containing DNA sequences.
	
	Optional parameters:
	  -in <file>          Input FASTA file. If unset, reads from STDIN.
	                      Default value: ''
	  -out <file>         Output file. If unset, writes to STDOUT.
	                      Default value: ''
	  -write_n <file>     Write BED file with N base coordinates
	                      Default value: ''
	  -write_other <file> Write BED file with other base coordinates
	                      Default value: ''
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastaInfo changelog
	FastaInfo 2024_08-36-g4fed1f49
	
	2024-08-16 Added optional parameters 'write_n' and 'write_other'.
	2015-06-25 initial version
[back to ngs-bits](https://github.com/imgag/ngs-bits)