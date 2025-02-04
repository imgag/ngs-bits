### VcfToBedpe tool help
	VcfToBedpe (2024_08-113-g94a3b440)
	
	Converts a VCF file containing structural variants to BEDPE format.
	
	Input can be MANTA oder DELLY VCF files.
	
	Mandatory parameters:
	  -in <file>        Input structural variant list in VCF format.
	  -out <file>       Output structural variant list in BEDPE format.
	
	Optional parameters:
	  -no_sort          Do not sort results
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfToBedpe changelog
	VcfToBedpe 2024_08-113-g94a3b440
	
	2023-03-23 Added support for Sniffles, cuteSV and dipdiff
[back to ngs-bits](https://github.com/imgag/ngs-bits)