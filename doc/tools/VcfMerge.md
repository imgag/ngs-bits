### VcfMerge tool help
	VcfMerge (2025_12-266-g396e1fe11)
	
	Merges several VCF files into a multi-sample VCF file.
	
	Input VCF have to be normalized (no multi-allelic variants, split into allelic primitives and indels left-aligned.
	The output has no information in the QUAL, FILTER and INFO column. It contains the following FORMAT entries: GT, DP, AF, GQ, PS, CT.
	Supported file formats for short-read are: freebayes, DRAGEN, DeepVariant.
	Supported file formats for long-read are: Clair3 (ONT), DeepVariant (PacBio)
	
	Mandatory parameters:
	  -in <filelist>           Input files to merge in VCF or VCG.GZ format.
	
	Optional parameters:
	  -out <file>              Output multi-sample VCF. If unset, writes to STDOUT.
	                           Default value: ''
	  -no_special_calls        Ignores special variant calls in input VCF files (mosaic, low-mappabilty, targeted, etc).
	                           Default value: 'false'
	  -min_qual <float>        If set, ignores input variants with less than the given QUAL cutoff.
	                           Default value: '0'
	  -bam <filelist>          Input BAM/CRAM files used for variant re-calling of uncalled variants. If not given, no re-calling is performed. For each 'in' file, a BAM file has to be provided in the same order.
	                           Default value: ''
	  -min_mapq <int>          Minimum mapping quality for re-calling.
	                           Default value: '20'
	  -no_genotype_correction  Do not perform genotype correction during re-calling, only calculate DP and AF.
	                           Default value: 'false'
	  -threads <int>           Number of threads used for re-calling
	                           Default value: '1'
	  -ref <file>              Reference genome FASTA file of BAM files. If unset 'reference_genome' from the 'settings.ini' file is used.
	                           Default value: ''
	  -long_read               Support long reads (> 1kb).
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### VcfMerge changelog
	VcfMerge 2025_12-266-g396e1fe11
	
	2026-05-05 Added 'long_read' parameter.
	2026-05-03 Added 'threads' parameter.
	2026-04-30 Added 'no_genotype_correction' parameter.
	2026-04-26 Added 'min_qual' and 'no_special_calls' parameters.
	2026-03-30 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)