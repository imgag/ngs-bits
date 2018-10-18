### VcfBreakMulti tool help
	VcfBreakMulti (2018_06-59-g24102d3)
	
	Breaks multi-allelic variants in several lines, preserving allele-specific INFO/SAMPLE fields.
	
	Optional parameters:
	  -in <file>   Input VCF file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output VCF list. If unset, writes to STDOUT.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfBreakMulti changelog
	VcfBreakMulti 2018_06-59-g24102d3
	
	2018-10-18 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)