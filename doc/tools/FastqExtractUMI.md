### FastqExtractUMI tool help
	FastqExtractUMI (2024_08-110-g317f43b9)
	
	Cuts UMI bases from the beginning of reads and adds them to read headers.
	
	Mandatory parameters:
	  -in1 <file>              Input FASTQ file 1.
	  -in2 <file>              Input FASTQ file 2.
	  -out1 <file>             Output filename for read 1 FASTQ.
	  -out2 <file>             Output filename for read 2 FASTQ.
	
	Optional parameters:
	  -cut1 <int>              Number of bases from the head of read 1 to use as UMI.
	                           Default value: '0'
	  -cut2 <int>              Number of bases from the head of read 2 to use as UMI.
	                           Default value: '0'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### FastqExtractUMI changelog
	FastqExtractUMI 2024_08-110-g317f43b9
	
	2020-07-15 Added 'compression_level' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)