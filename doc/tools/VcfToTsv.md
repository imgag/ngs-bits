### VcfToTsv tool help
	VcfToTsv (0.1-420-g3536bb0)
	
	Shifts indels in a variant list as far to the left as possible. Complex indels and multi-allelic deletions are not shifted!
	
	Mandatory parameters:
	  -in <file>   Input variant list in VCF format.
	  -out <file>  Output variant list in TSV format.
	
	Optional parameters:
	  -split       Split multi-allelic variants.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfToTsv changelog
	VcfToTsv 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)