### VcfCalculatePRS tool help
	VcfCalculatePRS (2025_03-80-g74f31dd7)
	
	Calculates the Polgenic Risk Score(s) for a sample.
	
	The PRS VCF files have to contain WEIGHT and POP_AF fields in the INFO column.
	Additionally some information about the PRS score is required in the VCF header.
	An example VCF file can be found at https://github.com/imgag/ngs-bits/blob/master/src/tools-TEST/data_in/VcfCalculatePRS_prs2.vcf
	
	Mandatory parameters:
	  -in <file>        Tabix indexed VCF.GZ file of a sample.
	  -prs <filelist>   List of PRS VCFs.
	  -bam <file>       BAM file corresponding to the VCF.
	  -out <file>       Output TSV file containing Scores and PRS details
	
	Optional parameters:
	  -details <file>   Output TSV containing each variant with weight, allele count and population AF.
	                    Default value: ''
	  -ref <file>       Reference genome FASTA file. If unset, 'reference_genome' from the 'settings.ini' file is used.
	                    Default value: ''
	  -min_depth <int>  Depth cutoff below which uncalled SNPs are considered not callable and POP_AF is used instead of genotype.
	                    Default value: '10'
	  -long_read        Support long reads (> 1kb).
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfCalculatePRS changelog
	VcfCalculatePRS 2025_03-80-g74f31dd7
	
	2025-05-21 Added parameter 'long_read' for long-read support.
	2024-06-05 Added support for imputed variants.
	2024-04-22 Added output of factors and support for wt variants.
	2022-12-15 Added BAM depth check and population AF.
	2020-07-22 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)