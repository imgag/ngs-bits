### CnvHunter tool help
	CnvHunter (0.1-222-g9be2128)
	
	Detects copy number variations from targeted resequencing data using non-matched control samples.
	
	Mandatory parameters:
	  -in <filelist>         Input TSV files (one per sample) containing coverage data (chr, start, end, avg_depth).
	  -out <file>            Output TSV file containing the detected CNVs.
	
	Optional parameters:
	  -out_reg <file>        If set, writes a BED file with region information (baq QC, excluded, good).
	                         Default value: ''
	  -n <int>               The number of most similar samples to consider.
	                         Default value: '20'
	  -exclude <file>        BED file with regions to exclude from the analysis.
	                         Default value: ''
	  -min_z <float>         Minimum z-score for CNV seed detection.
	                         Default value: '4'
	  -ext_min_z <float>     Minimum z-score for CNV extension around seeds.
	                         Default value: '2.5'
	  -ext_max_dist <int>    Maximum region distance for extension.
	                         Default value: '1000000'
	  -sam_min_depth <float> QC: Minimum average depth of a sample.
	                         Default value: '40'
	  -sam_min_corr <float>  QC: Minimum correlation of sample to constructed reference sample.
	                         Default value: '0.94999999999999996'
	  -sam_max_cnvs <int>    QC: Maximum number of CNV events in a sample.
	                         Default value: '30'
	  -reg_min_cov <float>   QC: Minimum (average) absolute depth of a target region.
	                         Default value: '20'
	  -reg_min_ncov <float>  QC: Minimum (average) normalized depth of a target region.
	                         Default value: '0.01'
	  -reg_max_cv <float>    QC: Maximum coefficient of variation (median/mad) of target region.
	                         Default value: '0.29999999999999999'
	  -verbose               Enables verbose mode. Writes detail information files for samples, regions and results.
	                         Default value: 'false'
	
	Special parameters:
	  --help                 Shows this help and exits.
	  --version              Prints version and exits.
	  --changelog            Prints changeloge and exits.
	  --tdx                  Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### CnvHunter changelog
	CnvHunter 0.1-222-g9be2128
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)