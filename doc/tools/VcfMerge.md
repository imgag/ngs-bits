### VcfMerge tool help
	VcfMerge (2025_12-175-gc2187ef59)
	
	Merges several VCF files into a multi-sample VCF file.
	
	Input VCF have to be normalized (no multi-allelic variants, split into allelic primitives and indels left-aligned.
	The output has no information in the QUAL, FILTER and INFO column. It contains the following FORMAT entries: GT, DP, AF, GQ, PS, CT.
	Supported file formats for short-read are: freebayes, DRAGEN, DeepVariant.
	Supported file formats for long-read are: Clair3 (ONT), DeepVariant (PacBio)
	
	Mandatory parameters:
	  -in <filelist>    Input files to merge in VCF or VCG.GZ format.
	
	Optional parameters:
	  -out <file>       Output multi-sample VCF. If unset, writes to STDOUT.
	                    Default value: ''
	  -trio             Enables trio mendelian error calculation. Expected sample order: child, father, mother.
	                    Default value: 'false'
	  -bam <filelist>   Input BAM/CRAM files used for variant re-calling of uncalled variants. For each 'in' file, a BAM file has to be provided in the same order.
	                    Default value: ''
	  -ref <file>       Reference genome FASTA file of BAM files. If unset 'reference_genome' from the 'settings.ini' file is used.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfMerge changelog
	VcfMerge 2025_12-175-gc2187ef59
	
	2026-03-30 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)