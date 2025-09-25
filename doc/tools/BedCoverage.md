### BedCoverage tool help
	BedCoverage (2025_07-127-g60fc6b39)
	
	Annotates a BED file with the average coverage of the regions from one or several BAM/CRAM file(s).
	
	Mandatory parameters:
	  -bam <filelist>   Input BAM/CRAM file(s).
	
	Optional parameters:
	  -min_mapq <int>   Minimum mapping quality.
	                    Default value: '1'
	  -in <file>        Input BED file. If unset, reads from STDIN.
	                    Default value: ''
	  -decimals <int>   Number of decimals used in output.
	                    Default value: '2'
	  -out <file>       Output BED file. If unset, writes to STDOUT.
	                    Default value: ''
	  -ref <file>       Reference genome for CRAM support (mandatory if CRAM is used).
	                    Default value: ''
	  -clear            Clear previous annotation columns before annotating (starting from 4th column).
	                    Default value: 'false'
	  -threads <int>    Number of threads used.
	                    Default value: '1'
	  -random_access    Use random access via index to get reads from BAM/CRAM instead of chromosome-wise sweep. Random access is quite slow, especially on CRAM, so use it only if a small subset of the file needs to be accessed.
	                    Default value: 'false'
	  -debug            Enable debug output.
	                    Default value: 'false'
	  -skip_mismapped   Skip reads with mapping quality less than 20 that are not properly paired (they are often mis-mapped).
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedCoverage changelog
	BedCoverage 2025_07-127-g60fc6b39
	
	2025-09-15 Added 'skip_mismapped' parameter.
	2024-06-26 Added 'random_access' parameter.
	2022-09-16 Added 'threads' parameter and removed 'dup' parameter.
	2022-08-12 Added parameter to clear previous annotation columns.
	2022-08-09 Removed mode parameter (panel mode is always used now).
	2020-11-27 Added CRAM support.
	2017-06-02 Added 'dup' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)