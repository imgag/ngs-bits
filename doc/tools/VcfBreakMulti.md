### VcfBreakMulti tool help
	VcfBreakMulti (2018_11-7-g60f117b)
	
	Breaks multi-allelic variants into several lines, making sure that allele-specific INFO/SAMPLE fields are still valid.
	
	Optional parameters:
	  -in <file>   Input VCF file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output VCF list. If unset, writes to STDOUT.
	               Default value: ''
	  -no_errors   Ignore VCF format errors if possible.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfBreakMulti changelog
	VcfBreakMulti 2018_11-7-g60f117b
	
	2018-10-18 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)