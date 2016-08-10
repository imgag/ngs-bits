### VcfLeftNormalize tool help
	VcfLeftNormalize (0.1-420-g3536bb0)
	
	Normalizes all variants and shifts indels to the left in a VCF file. Multi-allelic and complex variant are not changed!
	
	Optional parameters:
	  -in <file>   Input VCF file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output VCF list. If unset, writes to STDOUT.
	               Default value: ''
	  -ref <file>  Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfLeftNormalize changelog
	VcfLeftNormalize 0.1-420-g3536bb0
	
	2016-06-24 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)