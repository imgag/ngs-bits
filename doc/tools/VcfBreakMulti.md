### VcfBreakMulti tool help
	VcfBreakMulti (2024_08-113-g94a3b440)
	
	Breaks multi-allelic variants into several lines, making sure that allele-specific INFO/SAMPLE fields are still valid.
	
	Optional parameters:
	  -in <file>        Input VCF file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output VCF list. If unset, writes to STDOUT.
	                    Default value: ''
	  -no_errors        Ignore VCF format errors if possible.
	                    Default value: 'false'
	  -verbose          Writes ignored VCF format errors to stderr.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfBreakMulti changelog
	VcfBreakMulti 2024_08-113-g94a3b440
	
	2018-10-18 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)