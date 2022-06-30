### VcfAnnotateHexplorer tool help
	VcfAnnotateHexplorer (2022_04-120-g0b2ddab9)
	
	Annotates a VCF with Hexplorer and HBond scores.
	
	Mandatory parameters:
	  -out <file>  Output VCF file containing HEXplorer and HBOND scores in the INFO column.
	
	Optional parameters:
	  -in <file>   Input VCF file. If unset, reads from STDIN.
	               Default value: ''
	  -ref <file>  Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfAnnotateHexplorer changelog
	VcfAnnotateHexplorer 2022_04-120-g0b2ddab9
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)