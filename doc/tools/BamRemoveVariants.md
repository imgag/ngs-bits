### BamRemoveVariants tool help
	BamRemoveVariants (2024_08-36-g4fed1f49)
	
	Removes reads which contain the provided variants
	
	Mandatory parameters:
	  -in <file>   Input BAM/CRAM file.
	  -out <file>  Output BAM/CRAM file.
	  -vcf <file>  Input indexed VCF.GZ file.
	
	Optional parameters:
	  -ref <file>  Reference genome for CRAM support (mandatory if CRAM is used).
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BamRemoveVariants changelog
	BamRemoveVariants 2024_08-36-g4fed1f49
	
	2024-07-24 Inital commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)