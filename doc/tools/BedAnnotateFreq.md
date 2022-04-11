### BedAnnotateFreq tool help
	BedAnnotateFreq (2021_12-184-g566576b2)
	
	Extracts base counts and depth in the given regions from a BAM/CRAM files.
	
	Mandatory parameters:
	  -bam <filelist>  Input BAM/CRAM file(s).
	
	Optional parameters:
	  -in <file>       Input BED file. If unset, reads from STDIN.
	                   Default value: ''
	  -out <file>      Output TSV file. If unset, writes to STDOUT.
	                   Default value: ''
	  -ref <file>      Reference genome for CRAM support (mandatory if CRAM is used).
	                   Default value: ''
	  -min_mapq <int>  Minimum mapping quality.
	                   Default value: '1'
	  -min_baseq <int> Minimum base quality.
	                   Default value: '25'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --changelog      Prints changeloge and exits.
	  --tdx            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedAnnotateFreq changelog
	BedAnnotateFreq 2021_12-184-g566576b2
	
	2020-11-27 Added CRAM support.
[back to ngs-bits](https://github.com/imgag/ngs-bits)