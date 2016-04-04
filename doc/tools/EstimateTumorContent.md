### EstimateTumorContent tool help
	EstimateTumorContent (0.1-222-g9be2128)
	
	Estimates the tumor content using the median of the top-n somatic varaints.
	
	Mandatory parameters:
	  -tu <file>           Somatic variant list.
	  -tu_bam <file>       Tumor tissue BAM file.
	  -no_bam <file>       Normal tissue BAM file.
	
	Optional parameters:
	  -out <file>          Output TXT file. If unset, writes to STDOUT.
	                       Default value: ''
	  -min_depth <int>     Minmum depth in tumor and normal sample to consider a variant.
	                       Default value: '30'
	  -max_somatic <float> Maximum frequency in normal sample to consider a variant somatic.
	                       Default value: '0.01'
	  -n <int>             Minimal number of somatic autosomal heterocygous variants to calculate the tumor content.
	                       Default value: '10'
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### EstimateTumorContent changelog
	EstimateTumorContent 0.1-222-g9be2128
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)