### VcfBreakComplex tool help
	VcfBreakComplex (2018_11-39-g5d6be46)
	
	Breaks complex variants into primitives preserving INFO/SAMPLE fields.
	
	Multi-allelic variants are ignored, since we assume they have already been split, e.g. with VcfBreakMulti.
	Complex variants that are decomposed, are flagged with a BBC (before break-complex) info entry.
	
	Optional parameters:
	  -in <file>    Input VCF file. If unset, reads from STDIN.
	                Default value: ''
	  -out <file>   Output VCF list. If unset, writes to STDOUT.
	                Default value: ''
	  -stats <file> Ouptuts statistics. If unset, writes to STDERR.
	                Default value: ''
	  -keep_mnps    Write out MNPs unchanged.
	                Default value: 'false'
	  -no_tag       Skip annotation of decomposed variants with BBC in INFO field.
	                Default value: 'false'
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --changelog   Prints changeloge and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfBreakComplex changelog
	VcfBreakComplex 2018_11-39-g5d6be46
	
	2018-11-30 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)