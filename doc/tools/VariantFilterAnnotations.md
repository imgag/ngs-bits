### VariantFilterAnnotations tool help
	VariantFilterAnnotations (0.1-728-gb6bf123)
	
	Filter a variant list in GSvar format based on variant annotations.
	
	Mandatory parameters:
	  -in <file>                 Input variant list in GSvar format.
	  -out <file>                Output variant list. If unset, writes to STDOUT.
	
	Optional parameters:
	  -max_af <float>            Maximum overall allele frequency in public databases. '0.01' means 1% allele frequency!
	                             Default value: '-1'
	  -max_af_sub <float>        Maximum sub-population allele frequency in public databases. '0.01' means 1% allele frequency!
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
	  -geno_affected <enum>      If set, only variants with the specified genotype in affected samples pass. Performed after all other filters!
	                             Default value: 'any'
	                             Valid: 'hom,het,comphet,comphet+hom,any'
	  -geno_control <enum>       If set, only variants with the specified genotype in control samples pass. Performed after all other filters!
	                             Default value: 'any'
	                             Valid: 'hom,het,wt,not_hom,any'
	
	Special parameters:
	  --help                     Shows this help and exits.
	  --version                  Prints version and exits.
	  --changelog                Prints changeloge and exits.
	  --tdx                      Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantFilterAnnotations changelog
	VariantFilterAnnotations 0.1-728-gb6bf123
	
	2017-06-14 Refactoring of genotype-based filters: now also supports multi-sample filtering of affected and control samples.
	2017-06-14 Added sub-population allele frequency filter.
	2016-06-11 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)