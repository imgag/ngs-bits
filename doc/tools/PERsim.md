### PERsim tool help
	PERsim (0.1-256-g371df50)
	
	Paired-end read simulator for Illumina reads.
	
	Mandatory parameters:
	  -roi <file>      Target region BED file (the corresponding reference genome is taken from the settings.ini file).
	  -count <int>     Number of read pairs to generate.
	  -out1 <file>     Forward reads output file in .FASTQ.GZ format.
	  -out2 <file>     Reverse reads output file in .FASTQ.GZ format.
	
	Optional parameters:
	  -length <int>    Read length for forward/reverse reads.
	                   Default value: '100'
	  -ins_mean <int>  Library insert size mean value.
	                   Default value: '200'
	  -ins_stdev <int> Library insert size mean standard deviation.
	                   Default value: '70'
	  -error <float>   Base error probability (uniform distribution).
	                   Default value: '0.01'
	  -max_n <int>     Maximum number of N bases (from reference genome).
	                   Default value: '5'
	  -a1 <string>     Forward read sequencing adapter sequence (for read-through).
	                   Default value: 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCACGAGTTA'
	  -a2 <string>     Reverse read sequencing adapter sequence (for read-through).
	                   Default value: 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGTAGATCTC'
	  -ref <file>      Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                   Default value: ''
	  -v               Enable verbose debug output.
	                   Default value: 'false'
	
	Special parameters:
	  --help           Shows this help and exits.
	  --version        Prints version and exits.
	  --tdx            Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)