### VcfAnnotateHexplorer tool help
	VcfAnnotateHexplorer (2024_08-113-g94a3b440)
	
	Annotates a VCF with Hexplorer and HBond scores.
	
	Mandatory parameters:
	  -out <file>       Output VCF file containing HEXplorer and HBOND scores in the INFO column.
	
	Optional parameters:
	  -in <file>        Input VCF file. If unset, reads from STDIN.
	                    Default value: ''
	  -ref <file>       Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfAnnotateHexplorer changelog
	VcfAnnotateHexplorer 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)