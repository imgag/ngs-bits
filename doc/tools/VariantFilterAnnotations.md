### VariantFilterAnnotations tool help
	VariantFilterAnnotations (2018_06-27-gb6bdb9c)
	
	Filter a variant list in GSvar format based on variant annotations.
	
	The filter definition file list one filter per line using the following syntax:
	name[tab]param1=value[tab]param2=value...
	
	The order in the filter definition file defines the order in which the filters are applied.
	
	Several of the filters offer more than one action:
	  FILTER - Remove variants if they do not match the filter.
	  REMOVE - Remove variants if they match the filter.
	  KEEP - Force variants to be kept, even if filtered out by previous filter steps.
	
	The following filters are supported:
	Allele frequency                   Filter based on overall allele frequency of given by 1000 Genomes, ExAC and gnomAD.
	                                   Parameters:
	                                     max_af - Maximum allele frequency in % [default=1] [min=0.0] [max=100.0]
	Allele frequency (sub-populations) Filter based on sub-population allele frequency of given by ExAC.
	                                   Parameters:
	                                     max_af - Maximum allele frequency in % [default=1] [min=0.0] [max=100.0]
	Annotated pathogenic               Filter that matches variants annotated to be (likely) pathogenic by ClinVar or HGMD.
	                                   Parameters:
	                                     sources - Sources of pathogenicity to use [valid=ClinVar,HGMD] [non-empty]
	                                     action - Action to perform [default=KEEP] [valid=KEEP,FILTER]
	Classification NGSD                Filter for variant classification from NGSD.
	                                   Parameters:
	                                     classes - NGSD classes [valid=1,2,3,4,5,M] [non-empty]
	                                     action - Action to perform [default=KEEP] [valid=KEEP,FILTER,REMOVE]
	Column match                       Filter that matches the content of a column against a perl-compatible regular expression.
	                                   For details about regular expressions, see http://perldoc.perl.org/perlretut.html
	                                   Parameters:
	                                     pattern - Pattern to match to column [non-empty]
	                                     column - Column to filter [non-empty]
	                                     action - Action to perform [default=KEEP] [valid=KEEP,FILTER,REMOVE]
	Count NGSD                         Filter based on the hom/het occurances of a variant in the NGSD.
	                                   Parameters:
	                                     max_count - Maximum NGSD count [default=20] [min=0]
	                                     ignore_genotype - If set, all NGSD entries are counted independent of the variant genotype. Otherwise, for homozygous variants only homozygous NGSD entries are counted and for heterozygous variants all NGSD entries are counted. [default=false]
	Filter column empty                Filter that perserves variants which have no entry in the 'filter' column.
	Filter columns                     Filter based on the entries of the 'filter' column.
	                                   Parameters:
	                                     entries - Filter column entries [non-empty]
	                                     action - Action to perform [default=REMOVE] [valid=KEEP,REMOVE,FILTER]
	Gene inheritance                   Filter based on gene inheritance.
	                                   Parameters:
	                                     modes - Inheritance mode(s) [valid=AR,AD,XLR,XLD,MT] [non-empty]
	Gene pLI                           Filter based on the ExAC pLI score of genes.
	                                   Note that pLI score is most helpful for early-onset severe diseases.
	                                   Parameters:
	                                     min_score - Minumum score [default=0.90000000000000002] [min=0.0] [max=1.0]
	Genes                              Filter for that preserves a gene set.
	                                   Parameters:
	                                     genes - Gene set [non-empty]
	Genotype affected                  Filter for genotype(s) of the 'affected' sample(s).
	                                   Variants pass if 'affected' samples have the same genotype and the genotype is in the list selected genotype(s).
	                                   Parameters:
	                                     genotypes - Genotype(s) [valid=wt,het,hom,comp-het] [non-empty]
	Genotype control                   Filter for genotype of the 'control' sample(s).
	                                   Parameters:
	                                     genotypes - Genotype(s) [valid=wt,het,hom] [non-empty]
	                                     same_genotype - Also check that all 'control' samples have the same genotype. [default=false]
	Impact                             Filter based on the variant impact given by SnpEff.
	                                   For more details see: http://snpeff.sourceforge.net/SnpEff_manual.html#eff
	                                   Parameters:
	                                     impact - Valid impacts [valid=HIGH,MODERATE,LOW,MODIFIER] [non-empty]
	Predicted pathogenic               Filter for variants predicted to be pathogenic.
	                                   Prediction scores included are: phyloP=1.6, Sift=D, MetaLR=D, PolyPhen2=D, FATHMM=D and CADD=20.
	                                   Parameters:
	                                     min - Minimum number of pathogenic predictions [default=1] [min=1]
	                                     action - Action to perform [default=FILTER] [valid=KEEP,FILTER]
	SNPs only                          Filter that preserves SNPs and removes all other variant types.
	Text search                        Filter for text match in variant annotations.
	                                   The text comparison ignores the case.
	                                   Parameters:
	                                     term - Search term [non-empty]
	                                     action - Action to perform [default=FILTER] [valid=FILTER,KEEP,REMOVE]
	
	Mandatory parameters:
	  -in <file>      Input variant list in GSvar format.
	  -out <file>     Output variant list. If unset, writes to STDOUT.
	  -filters <file> Filter definition file.
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantFilterAnnotations changelog
	VariantFilterAnnotations 2018_06-27-gb6bdb9c
	
	2018-07-30 Replaced command-line parameters by INI file and added many new filters.
	2017-06-14 Refactoring of genotype-based filters: now also supports multi-sample filtering of affected and control samples.
	2017-06-14 Added sub-population allele frequency filter.
	2016-06-11 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)