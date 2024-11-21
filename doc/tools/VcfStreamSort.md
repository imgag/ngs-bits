### VcfStreamSort tool help
	VcfStreamSort (2024_08-113-g94a3b440)
	
	Sort entries of a VCF file according to genomic position using a stream. Variants must be grouped by chromosome!
	
	Optional parameters:
	  -in <file>        Input VCF file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output VCF list. If unset, writes to STDOUT.
	                    Default value: ''
	  -n <int>          Number of variants to cache for sorting.
	                    Default value: '10000'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfStreamSort changelog
	VcfStreamSort 2024_08-113-g94a3b440
	
	2019-01-08 Added REF, ALT and INFO fields to sorting for a defined output order.
	2016-06-27 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)