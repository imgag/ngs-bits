### BamToFastq tool help
	BamToFastq (2024_11-41-g79ac725e)
	
	Converts a coordinate-sorted BAM file to FASTQ files.
	
	Mandatory parameters:
	  -in <file>               Input BAM/CRAM file.
	  -out1 <file>             Read 1 output FASTQ.GZ file.
	
	Optional parameters:
	  -out2 <file>             Read 2 output FASTQ.GZ file (required for pair-end samples).
	                           Default value: ''
	  -reg <string>            Export only reads in the given region. Format: chr:start-end.
	                           Default value: ''
	  -remove_duplicates       Does not export reads marked as duplicates in SAM flags into the FASTQ file.
	                           Default value: 'false'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	  -write_buffer_size <int> Output write buffer size (number of FASTQ entry pairs).
	                           Default value: '100'
	  -ref <file>              Reference genome for CRAM support (mandatory if CRAM is used).
	                           Default value: ''
	  -extend <int>            Extend all reads to the given length. Base 'N' and base qualiy '2' are used for extension.
	                           Default value: '0'
	  -fix                     Keep only one read pair if several have the same name (note: needs much memory as read names are kept in memory).
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### BamToFastq changelog
	BamToFastq 2024_11-41-g79ac725e
	
	2024-12-13 Added 'fix' parameter.
	2024-12-09 Added 'extend' parameter.
	2023-03-22 Added mode for single-end samples (long reads).
	2020-11-27 Added CRAM support.
	2020-05-29 Massive speed-up by writing in background. Added 'compression_level' parameter.
	2020-03-21 Added 'reg' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)