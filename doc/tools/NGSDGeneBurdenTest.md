### NGSDGeneBurdenTest tool help
	NGSDGeneBurdenTest (2026_06-46-g72aab0308)
	
	Performs gene-wise Burden test on two sets of processed samples based on imported variants in the NGSD.
	
	Mandatory parameters:
	  -cases <file>             Text file containing case sample (one processed sample per line)
	  -controls <file>          Text file containing case sample (one processed sample per line)
	  -genes <file>             Text file containing genes to test (one gene per line)
	  -out <file>               Output TSV file containing the result of the Burden test.
	
	Optional parameters:
	  -max_ngsd_count <int>     Maximum NGSD count of a variant to still be included.
	                            Default value: '20'
	  -max_gnomad_af <float>    Maximum gnomAD allele frequency (in %) of a variant to still be included.
	                            Default value: '0.1'
	  -impacts <string>         Comma separated list of impacts which should be included (allowed values: HIGH, MODERATE, LOW, MODIFIER)
	                            Default value: 'HIGH,MODERATE'
	  -inheritance <string>     Inheritance mode to use. (allowed values: dominant, de-novo, recessive)
	                            Default value: 'dominant'
	  -include_mosaic           Include mosaic variants.
	                            Default value: 'false'
	  -predict_pathogenic       add variants with moderate/low/modifier impact only if CADD >= 20 or SpliceAI >= 0.5.
	                            Default value: 'false'
	  -include_cnvs             Include CNVs to test.
	                            Default value: 'false'
	  -ccr_only                 Limit test to constrained coding regions.
	                            Default value: 'false'
	  -splice_region_size <int> Extend coding region by this amount of bases.
	                            Default value: '20'
	  -excluded_regions <file>  BED file containing regions which should be excluded from the test.
	                            Default value: ''
	  -threads <int>            Number of threads used to perform the test.
	                            Default value: '4'
	  -test                     Uses the test database instead of on the production database.
	                            Default value: 'false'
	  -debug                    Activate debug output.
	                            Default value: 'false'
	  -skip_errors              Only report errors, do not fail execution.
	                            Default value: 'false'
	
	Special parameters:
	  --help                    Shows this help and exits.
	  --version                 Prints version and exits.
	  --changelog               Prints changeloge and exits.
	  --tdx                     Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]         Settings override file (no other settings files are used).
	
### NGSDGeneBurdenTest changelog
	NGSDGeneBurdenTest 2026_06-46-g72aab0308
	
	2026-06-01 Added live impact annotation.
	2026-05-22 Added multithreading.
	2026-05-20 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)