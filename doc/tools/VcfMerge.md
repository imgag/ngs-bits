### VcfMerge tool help
	VcfMerge (2024_08-113-g94a3b440)
	
	Merges several VCF files into one VCF
	
	Mandatory parameters:
	  -in <filelist>    Input VCF files that are merged. The VCF header is taken from the first file.
	
	Optional parameters:
	  -out <file>       Output VCF. If unset, writes to STDOUT.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfMerge changelog
	VcfMerge 2024_08-113-g94a3b440
	
	2023-12-14 Added support for gzipped input.
	2023-12-12 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)