### NGSDExportAnnotationData tool help
	NGSDExportAnnotationData (2024_08-110-g317f43b9)
	
	Export information aboug germline variants, somatic variants and genes form NGSD for use as annotation source, e.g. in megSAP.
	
	Optional parameters:
	  -germline <file>      Export germline variants (VCF format).
	                        Default value: ''
	  -somatic <file>       Export somatic variants (VCF format).
	                        Default value: ''
	  -genes <file>         Exports BED file containing genes and gene information.
	                        Default value: ''
	  -reference <file>     Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                        Default value: ''
	  -max_af <float>       Maximum allel frequency of exported variants (germline).
	                        Default value: '0.05'
	  -gene_offset <int>    Defines the number of bases by which the regions of genes are extended (genes).
	                        Default value: '5000'
	  -vicc_config_details  Includes details about VICC interpretation (somatic).
	                        Default value: 'false'
	  -threads <int>        Number of threads to use.
	                        Default value: '5'
	  -verbose              Enables verbose debug output.
	                        Default value: 'false'
	  -max_vcf_lines <int>  Maximum number of VCF lines to write per chromosome - for debugging.
	                        Default value: '-1'
	  -test                 Uses the test database instead of on the production database.
	                        Default value: 'false'
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]     Settings override file (no other settings files are used).
	
### NGSDExportAnnotationData changelog
	NGSDExportAnnotationData 2024_08-110-g317f43b9
	
	2023-06-18 Refactoring of command line parameters and parallelization of somatic export.
	2023-06-16 Added support for 'germline_mosaic' column in 'variant' table and added parallelization.
	2021-07-19 Code and parameter refactoring.
	2021-07-19 Added support for 'germline_het' and 'germline_hom' columns in 'variant' table.
	2019-12-06 Comments are now URL encoded.
	2019-09-25 Added somatic mode.
	2019-07-29 Added BED file for genes.
	2019-07-25 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)