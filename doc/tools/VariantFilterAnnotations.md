### VariantFilterAnnotations tool help
	VariantFilterAnnotations (0.1-699-g594d5da)
	
	Filter a variant list in GSvar format based on variant annotations.
	
	Mandatory parameters:
	  -in <file>                 Input variant list in GSvar format.
	  -out <file>                Output variant list. If unset, writes to STDOUT.
	
	Optional parameters:
	  -max_af <float>            Maximum allele frequency in public databases. '0.01' means 1% allele frequency!
	                             Default value: '-1'
	  -impact <string>           Comma-separated list of SnpEff impacts that pass.
	                             Default value: ''
	  -max_ihdb <int>            Maximum in-house database count.
	                             Default value: '-1'
	  -max_ihdb_ignore_genotype  If set, variant genotype is ignored. Otherwise, only homozygous database entries are counted for homozygous variants, and all entries are count for heterozygous variants.
	                             Default value: 'false'
	  -min_class <int>           Minimum classification of *classified* variants.
	                             Default value: '-1'
	  -filters <string>          Comma-separated list of filter column entries to remove.
	                             Default value: ''
	  -comphet                   If set, only hompound-heterozygous variants pass. Performed after all other filters!
	                             Default value: 'false'
	  -genotype <string>         If set, only variants with the specified genotype pass. Performed after all other filters!
	                             Default value: ''
	
	Special parameters:
	  --help                     Shows this help and exits.
	  --version                  Prints version and exits.
	  --changelog                Prints changeloge and exits.
	  --tdx                      Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantFilterAnnotations changelog
	VariantFilterAnnotations 0.1-699-g594d5da
	
	2016-06-11 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)