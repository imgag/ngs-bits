### CnvHunter tool help
	CnvHunter (0.1-852-g5a7f2d2)
	
	CNV detection from targeted resequencing data using non-matched control samples.
	
	Mandatory parameters:
	  -in <filelist>         Input TSV files (one per sample) containing coverage data (chr, start, end, avg_depth).
	  -out <file>            Output TSV file containing the detected CNVs.
	
	Optional parameters:
	  -n <int>               The number of most similar samples to use for reference construction.
	                         Default value: '30'
	  -min_z <float>         Minimum z-score for CNV seed detection.
	                         Default value: '4'
	  -ext_min_z <float>     Minimum z-score for CNV extension around seeds.
	                         Default value: '2'
	  -ext_gap_span <float>  Percentage of orignal region size that can be spanned while merging nearby regions (0 disables it).
	                         Default value: '20'
	  -sam_min_depth <float> QC: Minimum average depth of a sample.
	                         Default value: '40'
	  -sam_min_corr <float>  QC: Minimum correlation of sample to constructed reference sample.
	                         Default value: '0.94999999999999996'
	  -sam_corr_regs <int>   Maximum number of regions used for sample correlation calculation (to speed up comparisons for exoms etc.).
	                         Default value: '20000'
	  -reg_min_cov <float>   QC: Minimum (average) absolute depth of a target region.
	                         Default value: '20'
	  -reg_min_ncov <float>  QC: Minimum (average) normalized depth of a target region.
	                         Default value: '0.01'
	  -reg_max_cv <float>    QC: Maximum coefficient of variation (median/mad) of target region.
	                         Default value: '0.40000000000000002'
	  -debug <string>        Writes debug information for the sample matching the given name (or for all samples if 'ALL' is given).
	                         Default value: ''
	  -seg <string>          Writes a SEG file for the sample matching the given name (used for visualization in IGV).
	                         Default value: ''
	  -par <string>          Comma-separated list of pseudo-autosomal regions on the X chromosome.
	                         Default value: '1-2699520,154931044-155270560'
	  -cnp_file <file>       BED file containing copy-number-polymorphism (CNP) regions. They are excluded from the normalization/correlation calculation. E.g use the CNV map from http://dx.doi.org/10.1038/nrg3871.
	                         Default value: ''
	  -annotate <filelist>   List of BED files used for annotation. Each file adds a column to the output file. The base filename is used as colum name and 4th column of the BED file is used as annotation value.
	                         Default value: ''
	
	Special parameters:
	  --help                 Shows this help and exits.
	  --version              Prints version and exits.
	  --changelog            Prints changeloge and exits.
	  --tdx                  Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### CnvHunter changelog
	CnvHunter 0.1-852-g5a7f2d2
	
	2017-08-29 Updated default values of parameters 'n' and 'reg_max_cv' based on latest benchmarks.
	2017-08-28 Added generic annotation mechanism for annotation from BED files.
	2017-08-24 Added copy-number-polymorphisms regions input file ('cnp_file' parameter).
	2017-08-17 Added down-sampleing if input to speed up sample correlation ('sam_corr_regs' parameter).
	2017-08-15 Added allele frequency for each region to TSV output.
	2016-10-24 Added copy-number variant size to TSV output and added optional SEG output file.
	2016-09-01 Sample and region information files are now always written.
	2016-08-23 Added merging of large CNVs that were split to several regions due to noise.
	2016-08-21 Improved log output (to make parameter optimization easier).
[back to ngs-bits](https://github.com/imgag/ngs-bits)