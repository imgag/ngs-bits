### SvFilterAnnotations tool help
	SvFilterAnnotations (2025_07-53-gefb5888f)
	
	Filter a structural variant list in BEDPE format based on variant annotations.
	
	The filter definition file lists one filter per line using the following syntax:
	name[tab]param1=value[tab]param2=value...
	
	The order in the filter definition file defines the order in which the filters are applied.
	
	Several of the filters offer more than one action:
	  FILTER - Remove variants if they do not match the filter.
	  REMOVE - Remove variants if they match the filter.
	  KEEP - Force variants to be kept, even if filtered out by previous filter steps.
	
	The following filters are supported:
	SV CNV overlap              Filter the removes DEL/DUP without support from CNV calling.
	                            Parameters:
	                              min_ol - Minimum CNV overlap. [default=0.5] [min=0.0] [max=1.0]
	                              min_size - Minimum SV size in bases. [default=10000] [min=0]
	SV OMIM genes               Filter for OMIM genes i.e. the 'OMIM' column is not empty.
	                            Parameters:
	                              action - Action to perform [default=FILTER] [valid=REMOVE,FILTER]
	SV PE read depth            Show only SVs with at least a certain number of Paired End Reads
	                            (In trio/multi sample all (affected) samples must meet the requirements.)
	                            Parameters:
	                              PE Read Depth - minimal number of Paired End Reads [default=0] [min=0]
	                              only_affected - Apply filter only to affected Samples. [default=false]
	SV SomaticScore             Show only SVs with at least a certain Somaticscore
	                            Parameters:
	                              Somaticscore - min. Somaticscore [default=0] [min=0]
	SV allele frequency NGSD    Filter based on the allele frequency of this structural variant in the NGSD.
	                            Note: this filter should only be used for whole genome samples.
	                            Parameters:
	                              max_af - Maximum allele frequency in % [default=1] [min=0.0] [max=200.0]
	SV break point density NGSD Filter based on the density of SV break points in the NGSD in the CI of the structural variant.
	                            Parameters:
	                              max_density - Maximum density in the confidence interval of the SV [default=20] [min=0]
	                              remove_strict - Remove also SVs in which only one break point is above threshold. [default=false]
	                              only_system_specific - Filter only based on the density of breakpoint of the current processing system. [default=false]
	SV compound-heterozygous    Filter for compound-heterozygous SVs.
	                            Mode 'SV-SV' detects genes with two or more SV hits.
	                            Mode 'SV-SNV/INDEL' detects genes with at least one SV and at least one small variant hit (after other filters are applied).
	                            Parameters:
	                              mode - Compound-heterozygotes detection mode. [default=n/a] [valid=n/a,SV-SV,SV-SNV/INDEL]
	SV count NGSD               Filter based on the hom/het occurances of a structural variant in the NGSD.
	                            Parameters:
	                              max_count - Maximum NGSD SV count [default=20] [min=0]
	                              ignore_genotype - If set, all NGSD entries are counted independent of the variant genotype. Otherwise, for homozygous variants only homozygous NGSD entries are counted and for heterozygous variants all NGSD entries are counted. [default=false]
	SV filter columns           Filter structural variants based on the entries of the 'FILTER' column.
	                            Parameters:
	                              entries - Filter column entries [non-empty]
	                              action - Action to perform [default=REMOVE] [valid=REMOVE,FILTER,KEEP]
	SV gene constraint          Filter based on gene constraint (gnomAD o/e score for LOF variants).
	                            Note that gene constraint is most helpful for early-onset severe diseases.
	                            For details on gnomAD o/e, see https://macarthurlab.org/2018/10/17/gnomad-v2-1/
	                            Parameters:
	                              max_oe_lof - Maximum gnomAD o/e score for LoF variants [default=0.35] [min=0.0] [max=1.0]
	SV gene overlap             Filter based on gene overlap.
	                            Parameters:
	                              complete - Overlaps the complete gene. [default=true]
	                              exonic/splicing - Overlaps the coding or splicing region of the gene. [default=true]
	                              intronic/near gene - Overlaps the intronic region or less than 5kb up/down stream of the gene . [default=false]
	SV genotype affected        Filter structural variants (of affected samples) based on their genotype.
	                            Parameters:
	                              genotypes - Structural variant genotype(s) [valid=wt,het,hom,n/a] [non-empty]
	                              same_genotype - Also check that all 'control' samples have the same genotype. [default=false]
	SV genotype control         Filter structural variants of control samples based on their genotype.
	                            Parameters:
	                              genotypes - Structural variant genotype(s) [valid=wt,het,hom,n/a] [non-empty]
	                              same_genotype - Also check that all 'control' samples have the same genotype. [default=false]
	SV paired read AF           Show only SVs with a certain Paired Read Allele Frequency +/- 10%
	                            (In trio/multi sample all (affected) samples must meet the requirements.)
	                            Parameters:
	                              Paired Read AF - Paired Read Allele Frequency +/- 10% [default=0] [min=0.0] [max=1.0]
	                              only_affected - Apply filter only to affected Samples. [default=false]
	SV quality                  Filter structural variants based on their quality.
	                            Parameters:
	                              quality - Minimum quality score [default=0] [min=0]
	SV remove chr type          Removes all structural variants which contains non-standard/standard chromosomes.
	                            Parameters:
	                              chromosome type - Structural variants containing non-standard/standard chromosome are removed. [default=special chromosomes] [valid=special chromosomes,standard chromosomes] [non-empty]
	SV size                     Filter for SV size in the given range.
	                            Parameters:
	                              min_size - Minimum SV size (absolute size). [default=0] [min=0]
	                              max_size - Maximum SV size (absolute size). Select 0 for infinity. [default=0] [min=0]
	SV split read AF            Show only SVs with a certain Split Read Allele Frequency +/- 10%
	                            (In trio/multi sample all (affected) samples must meet the requirements.)
	                            Parameters:
	                              Split Read AF - Split Read Allele Frequency +/- 10% [default=0] [min=0.0] [max=1.0]
	                              only_affected - Apply filter only to affected Samples. [default=false]
	SV trio                     Filter trio structural variants
	                            Parameters:
	                              types - Variant types [default=de-novo,recessive,comp-het,LOH,x-linked] [valid=de-novo,recessive,comp-het,LOH,x-linked,imprinting]
	                              gender_child - Gender of the child - if 'n/a', the gender from the GSvar file header is taken [default=n/a] [valid=male,female,n/a]
	                              build - Genome build used for pseudoautosomal region coordinates [default=hg19] [valid=hg19,hg38]
	SV type                     Filter based on SV types.
	                            Parameters:
	                              Structural variant type - Structural variant type [valid=DEL,DUP,INS,INV,BND] [non-empty]
	SV-lr AF                    Show only (lr) SVs with a allele frequency between the given interval
	                            Parameters:
	                              min_af - minimal allele frequency [default=0] [min=0.0] [max=1.0]
	                              max_af - maximal allele frequency [default=1] [min=0.0] [max=1.0]
	SV-lr support reads         Show only (lr) SVs with a minimum number of supporting reads
	                            Parameters:
	                              min_support - Minimum support read count [default=5] [min=0] [max=10000]
	
	Mandatory parameters:
	  -in <file>        Input structural variant list in BEDPE format.
	  -out <file>       Output structural variant list in BEDPE format.
	  -filters <file>   Filter definition file.
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### SvFilterAnnotations changelog
	SvFilterAnnotations 2025_07-53-gefb5888f
	
	2020-04-16 Initial version of the tool. Based on VariantFilterAnnotations.
[back to ngs-bits](https://github.com/imgag/ngs-bits)