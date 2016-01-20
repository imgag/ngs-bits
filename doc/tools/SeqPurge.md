### SeqPurge tool help
	SeqPurge (0.1-184-gc4d2f1b)
	
	Removes adapter sequences from paired-end sequencing data.
	
	Mandatory parameters:
	  -in1 <filelist>     Forward input gzipped FASTQ file(s).
	  -in2 <filelist>     Reverse input gzipped FASTQ file(s).
	  -out1 <file>        Forward output gzipped FASTQ file.
	  -out2 <file>        Reverse output gzipped FASTQ file.
	
	Optional parameters:
	  -a1 <string>        Forward adapter sequence (at least 15 bases).
	                      Default value: 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCACGAGTTA'
	  -a2 <string>        Reverse adapter sequence (at least 15 bases).
	                      Default value: 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGTAGATCTC'
	  -match_perc <float> Minimum percentage of matching bases for sequence/adapter matches.
	                      Default value: '80'
	  -mep <float>        Maximum error probability of sequence matches.
	                      Default value: '1.0000000000000001e-05'
	  -qcut <int>         Quality trimming cutoff for trimming from the end of reads using a sliding mindow approach. Set to 0 to disable.
	                      Default value: '15'
	  -qwin <int>         Quality trimming window size.
	                      Default value: '5'
	  -qoff <int>         Quality trimming FASTQ score offset.
	                      Default value: '33'
	  -ncut <int>         Number of subsequent Ns to trimmed using a sliding mindow approach from the front of reads. Set to 0 to disable.
	                      Default value: '7'
	  -min_len <int>      Minimum read length after adapter trimming. Shorter reads are discarded.
	                      Default value: '15'
	  -threads <int>      The number of threads used for trimming (an additional thread is used for reading data).
	                      Default value: '1'
	  -out3 <file>        Name prefix of singleton read output files (if only one read of a pair is discarded).
	                      Default value: ''
	  -summary <file>     Write summary/progress to this file instead of STDOUT.
	                      Default value: ''
	  -qc <file>          If set, a read QC file in qcML format is created (just like ReadQC).
	                      Default value: ''
	  -prefetch <int>     Maximum number of reads that may be pre-fetched to speed up trimming
	                      Default value: '1000'
	  -debug              Enables debug output (use only with one thread).
	                      Default value: 'false'
	  -progress           Enables progress output.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --tdx               Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)