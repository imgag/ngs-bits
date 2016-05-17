### SeqPurge tool help
	SeqPurge (0.1-327-g026bb74)
	
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
	  -mep <float>        Maximum error probability of insert and adapter matches.
	                      Default value: '9.9999999999999995e-07'
	  -qcut <int>         Quality trimming cutoff for trimming from the end of reads using a sliding window approach. Set to 0 to disable.
	                      Default value: '15'
	  -qwin <int>         Quality trimming window size.
	                      Default value: '5'
	  -qoff <int>         Quality trimming FASTQ score offset.
	                      Default value: '33'
	  -ncut <int>         Number of subsequent Ns to trimmed using a sliding window approach from the front of reads. Set to 0 to disable.
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
	  -ec                 Enable error-correction of adapter-trimmed reads (only those with insert match).
	                      Default value: 'false'
	  -debug              Enables debug output (use only with one thread).
	                      Default value: 'false'
	  -progress           Enables progress output.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SeqPurge changelog
	SeqPurge 0.1-327-g026bb74
	
	2016-04-15 Removed large part of the overtrimming described in the paper (~75% of reads overtrimmed, ~50% of bases overtrimmed).
	2016-04-06 Added error correction (optional).
	2016-03-16 Version used in the SeqPurge paper: http://bmcbioinformatics.biomedcentral.com/articles/10.1186/s12859-016-1069-7
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)