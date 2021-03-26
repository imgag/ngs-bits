### VariantFilterAnnotations tool help
	VariantFilterAnnotations (2021_03-23-g5c26fea8)
	
	Filter a variant list in GSvar format based on variant annotations.
	
	The filter definition file lists one filter per line using the following syntax:
	name[tab]param1=value[tab]param2=value...
	
	The order in the filter definition file defines the order in which the filters are applied.
	
	Several of the filters offer more than one action:
	  FILTER - Remove variants if they do not match the filter.
	  REMOVE - Remove variants if they match the filter.
	  KEEP - Force variants to be kept, even if filtered out by previous filter steps.
	
	The following filters are supported:
	Allele frequency                   Filter based on overall allele frequency given by 1000 Genomes and gnomAD.
	                                   Parameters:
	                                     max_af - Maximum allele frequency in % [default=1] [min=0.0] [max=100.0]
	Allele frequency (sub-populations) Filter based on sub-population allele frequency given by gnomAD.
	                                   Parameters:
	                                     max_af - Maximum allele frequency in % [default=1] [min=0.0] [max=100.0]
	Annotated pathogenic               Filter that matches variants annotated to be pathogenic by ClinVar or HGMD.
	                                   Parameters:
	                                     sources - Sources of pathogenicity to use [default=ClinVar,HGMD] [valid=ClinVar,HGMD] [non-empty]
	                                     also_likely_pathogenic - Also consider likely pathogenic variants [default=false]
	                                     action - Action to perform [default=KEEP] [valid=KEEP,FILTER]
	Classification NGSD                Filter for variant classification from NGSD.
	                                   Parameters:
	                                     classes - NGSD classes [default=4,5] [valid=1,2,3,4,5,M] [non-empty]
	                                     action - Action to perform [default=KEEP] [valid=KEEP,FILTER,REMOVE]
	Column match                       Filter that matches the content of a column against a perl-compatible regular expression.
	                                   For details about regular expressions, see http://perldoc.perl.org/perlretut.html
	                                   Parameters:
	                                     pattern - Pattern to match to column [non-empty]
	                                     column - Column to filter [non-empty]
	                                     action - Action to perform [default=KEEP] [valid=KEEP,FILTER,REMOVE]
	Conservedness                      Filter for variants that affect conserved bases
	                                   Parameters:
	                                     min_score - Minimum phlyoP score. [default=1.6]
	Count NGSD                         Filter based on the hom/het occurances of a variant in the NGSD.
	                                   Parameters:
	                                     max_count - Maximum NGSD count [default=20] [min=0]
	                                     ignore_genotype - If set, all NGSD entries are counted independent of the variant genotype. Otherwise, for homozygous variants only homozygous NGSD entries are counted and for heterozygous variants all NGSD entries are counted. [default=false]
	Filter column empty                Filter that perserves variants which have no entry in the 'filter' column.
	Filter columns                     Filter based on the entries of the 'filter' column.
	                                   Parameters:
	                                     entries - Filter column entries [non-empty]
	                                     action - Action to perform [default=REMOVE] [valid=KEEP,REMOVE,FILTER]
	GSvar score/rank                   Filter based GSvar score/rank.
	                                   Parameters:
	                                     top - Show top X rankging variants only. [default=10] [min=1]
	Gene constraint                    Filter based on gene constraint (gnomAD o/e score for LOF variants).
	                                   Note that gene constraint is most helpful for early-onset severe diseases.
	                                   For details on gnomAD o/e, see https://macarthurlab.org/2018/10/17/gnomad-v2-1/
	                                   Note: ExAC pLI is deprected and support for backward compatibility with old GSvar files.
	                                   Parameters:
	                                     max_oe_lof - Maximum gnomAD o/e score for LoF variants [default=0.35] [min=0.0] [max=1.0]
	                                     min_pli - Minumum ExAC pLI score [default=0.9] [min=0.0] [max=1.0]
	Gene inheritance                   Filter based on gene inheritance.
	                                   Parameters:
	                                     modes - Inheritance mode(s) [valid=AR,AD,XLR,XLD,MT] [non-empty]
	Genes                              Filter for that preserves a gene set.
	                                   Parameters:
	                                     genes - Gene set [non-empty]
	Genotype affected                  Filter for genotype(s) of the 'affected' sample(s).
	                                   Variants pass if 'affected' samples have the same genotype and the genotype is in the list selected genotype(s).
	                                   Parameters:
	                                     genotypes - Genotype(s) [valid=wt,het,hom,n/a,comp-het] [non-empty]
	Genotype control                   Filter for genotype of the 'control' sample(s).
	                                   Parameters:
	                                     genotypes - Genotype(s) [valid=wt,het,hom,n/a] [non-empty]
	                                     same_genotype - Also check that all 'control' samples have the same genotype. [default=false]
	Impact                             Filter based on the variant impact given by VEP.
	                                   For more details see: https://www.ensembl.org/info/genome/variation/prediction/predicted_data.html
	                                   Parameters:
	                                     impact - Valid impacts [default=HIGH,MODERATE,LOW] [valid=HIGH,MODERATE,LOW,MODIFIER] [non-empty]
	OMIM genes                         Filter for OMIM genes i.e. the 'OMIM' column is not empty.
	                                   Parameters:
	                                     action - Action to perform [default=FILTER] [valid=REMOVE,FILTER]
	Predicted pathogenic               Filter for variants predicted to be pathogenic.
	                                   Prediction scores included are: phyloP>=1.6, Sift=D, PolyPhen=D, fathmm-MKL>=0.5, CADD>=20 and REVEL>=0.5.
	                                   Parameters:
	                                     min - Minimum number of pathogenic predictions [default=1] [min=1]
	                                     action - Action to perform [default=FILTER] [valid=KEEP,FILTER]
	Regulatory                         Filter for regulatory variants, i.e. the 'regulatory' column is not empty.
	                                   Parameters:
	                                     action - Action to perform [default=FILTER] [valid=REMOVE,FILTER]
	SNVs only                          Filter that preserves SNVs and removes all other variant types.
	                                   Parameters:
	                                     invert - If set, removes all SNVs and keeps all other variants. [default=false]
	Somatic allele frequency           Filter based on the allele frequency of variants in tumor/normal samples.
	                                   Parameters:
	                                     min_af_tum - Minimum allele frequency in tumor sample [%] [default=5] [min=0.0] [max=100.0]
	                                     max_af_nor - Maximum allele frequency in normal sample [%] [default=1] [min=0.0] [max=100.0]
	Text search                        Filter for text match in variant annotations.
	                                   The text comparison ignores the case.
	                                   Parameters:
	                                     term - Search term [non-empty]
	                                     action - Action to perform [default=FILTER] [valid=FILTER,KEEP,REMOVE]
	Trio                               Filter trio variants
	                                   Parameters:
	                                     types - Variant types [default=de-novo,recessive,comp-het,LOH,x-linked] [valid=de-novo,recessive,comp-het,LOH,x-linked,imprinting]
	                                     gender_child - Gender of the child - if 'n/a', the gender from the GSvar file header is taken [default=n/a] [valid=male,female,n/a]
	Tumor zygosity                     Filter based on the zygosity of tumor-only samples. Filters out germline het/hom calls.
	                                   Parameters:
	                                     het_af_range - Consider allele frequencies of 50% ± het_af_range as heterozygous and thus as germline. [default=0] [min=0] [max=49.9]
	                                     hom_af_range - Consider allele frequencies of 100% ± hom_af_range as homozygous and thus as germline. [default=0] [min=0] [max=99.9]
	Variant quality                    Filter for variant quality
	                                   Parameters:
	                                     qual - Minimum variant quality score (Phred) [default=250] [min=0]
	                                     depth - Minimum depth [default=0] [min=0]
	                                     mapq - Minimum mapping quality of alternate allele (Phred) [default=40] [min=0]
	                                     strand_bias - Maximum strand bias Phred score of alternate allele (set -1 to disable) [default=20] [min=-1]
	                                     allele_balance - Maximum allele balance Phred score (set -1 to disable) [default=40] [min=-1]
	Variant type                       Filter for variant types as defined by sequence ontology.
	                                   For details see http://www.sequenceontology.org/browser/obob.cgi
	                                   Parameters:
	                                     HIGH - High impact variant types [default=frameshift_variant,splice_acceptor_variant,splice_donor_variant,start_lost,start_retained_variant,stop_gained,stop_lost] [valid=frameshift_variant,splice_acceptor_variant,splice_donor_variant,start_lost,start_retained_variant,stop_gained,stop_lost]
	                                     MODERATE - Moderate impact variant types [default=inframe_deletion,inframe_insertion,missense_variant] [valid=inframe_deletion,inframe_insertion,missense_variant]
	                                     LOW - Low impact variant types [default=splice_region_variant] [valid=splice_region_variant,stop_retained_variant,synonymous_variant]
	                                     MODIFIER - Lowest impact variant types [valid=3_prime_UTR_variant,5_prime_UTR_variant,NMD_transcript_variant,downstream_gene_variant,intergenic_variant,intron_variant,mature_miRNA_variant,non_coding_transcript_exon_variant,non_coding_transcript_variant,upstream_gene_variant]
	
	Mandatory parameters:
	  -in <file>      Input variant list in GSvar format.
	  -out <file>     Output variant list in GSvar format.
	  -filters <file> Filter definition file.
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantFilterAnnotations changelog
	VariantFilterAnnotations 2021_03-23-g5c26fea8
	
	2018-07-30 Replaced command-line parameters by INI file and added many new filters.
	2017-06-14 Refactoring of genotype-based filters: now also supports multi-sample filtering of affected and control samples.
	2017-06-14 Added sub-population allele frequency filter.
	2016-06-11 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)