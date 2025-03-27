### VcfSplit tool help
	VcfSplit (2024_08-113-g94a3b440)
	
	Splits a VCF into several chunks
	
	Mandatory parameters:
	  -lines <int>      Number of variant lines per chunk.
	  -out <string>     Output VCF base name. Suffixed with chunk number and extension, e.g. '0001.vcf'
	
	Optional parameters:
	  -in <file>        Input VCF file. If unset, reads from STDIN.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfSplit changelog
	VcfSplit 2024_08-113-g94a3b440
	
	2023-12-10 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)