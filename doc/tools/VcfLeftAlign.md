### VcfLeftAlign tool help
	VcfLeftAlign (0.1-222-g9be2128)
	
	Shifts indels in a variant list as far to the left as possible. Complex indels and multi-allelic deletions are not shifted!
	
	Mandatory parameters:
	  -in <file>   Input variant list.
	  -out <file>  Output variant list.
	
	Optional parameters:
	  -ref <file>  Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfLeftAlign changelog
	VcfLeftAlign 0.1-222-g9be2128
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)