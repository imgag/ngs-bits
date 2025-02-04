### FastqDownsample tool help
	FastqDownsample (2024_08-110-g317f43b9)
	
	Downsamples paired-end FASTQ files.
	
	Mandatory parameters:
	  -in1 <file>              Forward input gzipped FASTQ file(s).
	  -in2 <file>              Reverse input gzipped FASTQ file(s).
	  -percentage <float>      Percentage of reads to keep.
	  -out1 <file>             Forward output gzipped FASTQ file.
	  -out2 <file>             Reverse output gzipped FASTQ file.
	
	Optional parameters:
	  -test                    Test mode: fix random number generator seed and write kept read names to STDOUT.
	                           Default value: 'false'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### FastqDownsample changelog
	FastqDownsample 2024_08-110-g317f43b9
	
	2020-07-15 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)