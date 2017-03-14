### VariantAnnotateStrand tool help
	VariantAnnotateStrand (0.1-645-gfc628cd)
	
	Annotates strand and family information to variants in a vcf file.
	
	Annotates strand information to variants in a vcf file.
	BAM file has to be annotated using BamDeduplicateByBarcode.
	
	Mandatory parameters:
	  -vcf <file>    Input VCF file.
	  -bam <file>    Input BAM file.
	  -out <file>    Output VCF file.
	
	Optional parameters:
	  -mip <file>    Input MIP file.
	                 Default value: ''
	  -hpHS <file>   Input HaloPlex HS file.
	                 Default value: ''
	  -name <string> Column name.
	                 Default value: ''
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantAnnotateStrand changelog
	VariantAnnotateStrand 0.1-645-gfc628cd
	
	2017-01-17 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)