### SnifflesVcfFix tool help
	SnifflesVcfFix (2025_01-25-g1d2b52ea)
	
	Fixes VCF file from Sniffles SV Caller.
	
	Converts lowEvidence variants into het variants.
	
	Optional parameters:
	  -in <file>        Input VCF file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output VCF list. If unset, writes to STDOUT.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### SnifflesVcfFix changelog
	SnifflesVcfFix 2025_01-25-g1d2b52ea
	
	2025-02-14 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)