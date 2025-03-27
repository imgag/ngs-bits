### VcfExtractSamples tool help
	VcfExtractSamples (2024_08-110-g317f43b9)
	
	Extract one or several samples from a VCF file. Can also be used to re-order sample columns.
	
	Mandatory parameters:
	  -samples <string> Comma-separated list of samples to extract (in the given order).
	
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
	
### VcfExtractSamples changelog
	VcfExtractSamples 2024_08-110-g317f43b9
	
	2018-11-27 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)