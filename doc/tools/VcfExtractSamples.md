### VcfExtractSamples tool help
	VcfExtractSamples (2018_11-7-g60f117b)
	
	Extract one or several samples from a VCF file.
	
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
	
### VcfExtractSamples changelog
	VcfExtractSamples 2018_11-7-g60f117b
	
	2018-11-27 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)