### Hexplorer tool help
	Hexplorer (2022_04-156-ga57fe9df)
	
	Calculate HEXplorer & HBOND splicing scores.
	
	Mandatory parameters:
	  -out <file>  Output file containing HEXplorer & HBOND scores. For vcf file input the scores will be the in INFO column and the HEXplorer scores will be the change between WT and MT (HEXplorer score normalized by sequence length MT-WT).
	
	Optional parameters:
	  -ref <file>  Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	               Default value: ''
	  -in <file>   Input file. If this is not a vcf file it should contain one sequence per line. If unset, reads from STDIN.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### Hexplorer changelog
	Hexplorer 2022_04-156-ga57fe9df
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)