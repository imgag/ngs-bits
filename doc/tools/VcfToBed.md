### VcfToBed tool help
	VcfToBed (2024_08-113-g94a3b440)
	
	Converts a VCF file to a BED file.
	
	For SNVs exactly one base is in the output region.
	For insertions the base before and after the insertion is contained.
	For deletions the base before the deletion and all deleted bases are contained.
	
	Optional parameters:
	  -in <file>        Input variant list in VCF format.
	                    Default value: ''
	  -out <file>       Output region in BED format.
	                    Default value: ''
	  -add_chr          Add 'chr' to chromosome names if missing.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfToBed changelog
	VcfToBed 2024_08-113-g94a3b440
	
	2022-09-13 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)