### SeqPurge tool help
	SeqPurge (2024_08-110-g317f43b9)
	
	Removes adapter sequences from paired-end sequencing data.
	
	Mandatory parameters:
	  -in1 <filelist>          Forward input gzipped FASTQ file(s).
	  -in2 <filelist>          Reverse input gzipped FASTQ file(s).
	  -out1 <file>             Forward output gzipped FASTQ file.
	  -out2 <file>             Reverse output gzipped FASTQ file.
	
	Optional parameters:
	  -a1 <string>             Forward adapter sequence (at least 15 bases).
	                           Default value: 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCA'
	  -a2 <string>             Reverse adapter sequence (at least 15 bases).
	                           Default value: 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT'
	  -match_perc <float>      Minimum percentage of matching bases for sequence/adapter matches.
	                           Default value: '80'
	  -mep <float>             Maximum error probability of insert and adapter matches.
	                           Default value: '1e-06'
	  -qcut <int>              Quality trimming cutoff for trimming from the end of reads using a sliding window approach. Set to 0 to disable.
	                           Default value: '15'
	  -qwin <int>              Quality trimming window size.
	                           Default value: '5'
	  -qoff <int>              Quality trimming FASTQ score offset.
	                           Default value: '33'
	  -ncut <int>              Number of subsequent Ns to trimmed using a sliding window approach from the front of reads. Set to 0 to disable.
	                           Default value: '7'
	  -min_len <int>           Minimum read length after adapter trimming. Shorter reads are discarded.
	                           Default value: '30'
	  -threads <int>           The number of threads used for trimming (up to three additional threads are used for reading and writing).
	                           Default value: '1'
	  -out3 <file>             Name prefix of singleton read output files (if only one read of a pair is discarded).
	                           Default value: ''
	  -summary <file>          Write summary/progress to this file instead of STDOUT.
	                           Default value: ''
	  -qc <file>               If set, a read QC file in qcML format is created (just like ReadQC).
	                           Default value: ''
	  -block_size <int>        Number of FASTQ entries processed in one block.
	                           Default value: '10000'
	  -block_prefetch <int>    Number of blocks that may be pre-fetched into memory.
	                           Default value: '32'
	  -ec                      Enable error-correction of adapter-trimmed reads (only those with insert match).
	                           Default value: 'false'
	  -debug                   Enables debug output (use only with one thread).
	                           Default value: 'false'
	  -progress <int>          Enables progress output at the given interval in milliseconds (disabled by default).
	                           Default value: '-1'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### SeqPurge changelog
	SeqPurge 2024_08-110-g317f43b9
	
	2022-07-15 Improved scaling with more than 4 threads and CPU usage.
	2019-03-26 Added 'compression_level' parameter.
	2019-02-11 Added writer thread to make SeqPurge scale better when using many threads.
	2017-06-15 Changed default value of 'min_len' parameter from 15 to 30.
	2016-08-10 Fixed bug in binomial calculation (issue #1).
	2016-04-15 Removed large part of the overtrimming described in the paper (~75% of reads overtrimmed, ~50% of bases overtrimmed).
	2016-04-06 Added error correction (optional).
	2016-03-16 Version used in the SeqPurge paper: http://bmcbioinformatics.biomedcentral.com/articles/10.1186/s12859-016-1069-7
[back to ngs-bits](https://github.com/imgag/ngs-bits)