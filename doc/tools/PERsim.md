### PERsim tool help
	PERsim (2024_08-110-g317f43b9)
	
	Paired-end read simulator for Illumina reads.
	
	PERsim generates paired-end reads of a given length for a region of interest in the genome:
	 - insert size is modelled using a gaussian distribution.
	 - read-through into the sequencing adapters is modelled.
	 - sequencing errors are modelled using a simple uniform distribution.
	
	Mandatory parameters:
	  -roi <file>              Target region BED file.
	  -count <int>             Number of read pairs to generate.
	  -out1 <file>             Forward reads output file in .FASTQ.GZ format.
	  -out2 <file>             Reverse reads output file in .FASTQ.GZ format.
	
	Optional parameters:
	  -length <int>            Read length for forward/reverse reads.
	                           Default value: '100'
	  -ins_mean <int>          Library insert size mean value.
	                           Default value: '200'
	  -ins_stdev <int>         Library insert size mean standard deviation.
	                           Default value: '70'
	  -error <float>           Base error probability (uniform distribution).
	                           Default value: '0.01'
	  -max_n <int>             Maximum number of N bases (from reference genome).
	                           Default value: '5'
	  -a1 <string>             Forward read sequencing adapter sequence (for read-through).
	                           Default value: 'AGATCGGAAGAGCACACGTCTGAACTCCAGTCACGAGTTA'
	  -a2 <string>             Reverse read sequencing adapter sequence (for read-through).
	                           Default value: 'AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGTAGATCTC'
	  -ref <file>              Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                           Default value: ''
	  -v                       Enable verbose debug output.
	                           Default value: 'false'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### PERsim changelog
	PERsim 2024_08-110-g317f43b9
	
	2020-07-15 Added 'compression_level' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)