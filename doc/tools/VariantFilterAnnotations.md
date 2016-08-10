### VariantFilterAnnotations tool help
	VariantFilterAnnotations (0.1-420-g3536bb0)
	
	Filter a variant list based on variant annotations.
	
	Mandatory parameters:
	  -in <file>         Input variant list.
	  -out <file>        Output variant list. If unset, writes to STDOUT.
	
	Optional parameters:
	  -max_af <float>    Maximum allele frequency in public databases. '0.01' means 1% allele frequency!
	                     Default value: '-1'
	  -impact <string>   Comma-separated list of SnpEff impacts that pass.
	                     Default value: ''
	  -max_ihdb <int>    Maximum in-house database frequency. For homozygous variants, only homozygous database entries count. For heterozygous variants all entries count.
	                     Default value: '-1'
	  -min_class <int>   Minimum classification of *classified* variants.
	                     Default value: '-1'
	  -filters <string>  Comma-separated list of filter column entries to remove.
	                     Default value: ''
	  -comphet           If set, only hompound-heterozygous variants pass of the *heterozygous* variants. Performed after all other filters!
	                     Default value: 'false'
	  -genotype <string> If set, only variants with that genotype pass. Performed after all other filters!
	                     Default value: ''
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantFilterAnnotations changelog
	VariantFilterAnnotations 0.1-420-g3536bb0
	
	2016-06-11 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)