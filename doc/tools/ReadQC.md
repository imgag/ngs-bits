### ReadQC tool help
	ReadQC (2023_03-63-gec44de43)
	
	Calculates QC metrics on unprocessed NGS reads.
	
	Mandatory parameters:
	  -in1 <filelist>          Forward input gzipped FASTQ file(s).
	
	Optional parameters:
	  -in2 <filelist>          Reverse input gzipped FASTQ file(s) for paired-end mode (same number of cycles/reads as 'in1').
	                           Default value: ''
	  -out <file>              Output qcML file. If unset, writes to STDOUT.
	                           Default value: ''
	  -txt                     Writes TXT format instead of qcML.
	                           Default value: 'false'
	  -out1 <file>             If set, writes merged forward FASTQs to this file (gzipped).
	                           Default value: ''
	  -out2 <file>             If set, writes merged reverse FASTQs to this file (gzipped)
	                           Default value: ''
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	  -long_read               Support long reads (> 1kb).
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### ReadQC changelog
	ReadQC 2023_03-63-gec44de43
	
	2023-04-18 Added support for LongRead
	2021-02-03 Added option to write out merged input FASTQs (out1/out2).
	2016-08-19 Added support for multiple input files.
[back to ngs-bits](https://github.com/imgag/ngs-bits)