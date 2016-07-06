### VcfStreamSort tool help
	VcfStreamSort (0.1-382-g4694ff9)
	
	Sort entries of a VCF file according to genomic position using a stream. Variants must be grouped by chromosome!
	
	Optional parameters:
	  -in <file>   Input VCF file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output VCF list. If unset, writes to STDOUT.
	               Default value: ''
	  -n <int>     Number of variants to cache for sorting.
	               Default value: '10000'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfStreamSort changelog
	VcfStreamSort 0.1-382-g4694ff9
	
	2016-06-27 Initial implementation.
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)