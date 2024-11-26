### SampleAncestry tool help
	SampleAncestry (2024_08-110-g317f43b9)
	
	Estimates the ancestry of a sample based on variants.
	
	Mandatory parameters:
	  -in <filelist>        Input variant list(s) in VCF or VCF.GZ format.
	
	Optional parameters:
	  -out <file>           Output TSV file. If unset, writes to STDOUT.
	                        Default value: ''
	  -min_snps <int>       Minimum number of informative SNPs for population determination. If less SNPs are found, 'NOT_ENOUGH_SNPS' is returned.
	                        Default value: '1000'
	  -score_cutoff <float> Absolute score cutoff above which a sample is assigned to a population.
	                        Default value: '0.32'
	  -mad_dist <float>     Maximum number of median average diviations that are allowed from median population score.
	                        Default value: '4.2'
	  -build <enum>         Genome build used to generate the input.
	                        Default value: 'hg38'
	                        Valid: 'hg19,hg38'
	
	Special parameters:
	  --help                Shows this help and exits.
	  --version             Prints version and exits.
	  --changelog           Prints changeloge and exits.
	  --tdx                 Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]     Settings override file (no other settings files are used).
	
### SampleAncestry changelog
	SampleAncestry 2024_08-110-g317f43b9
	
	2021-05-17 Population assignment is based on abolute score and on median/mad now. Should be much more accurate now especially for admixed samples.
	2020-08-07 VCF files only as input format for variant list.
	2018-12-10 Fixed bug in handling of 'pop_dist' parameter.
	2018-07-11 Added build switch for hg38 support.
	2018-07-03 First version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)