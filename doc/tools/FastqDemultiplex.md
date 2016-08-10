### FastqDemultiplex tool help
	FastqDemultiplex (0.1-420-g3536bb0)
	
	Demultiplexes FASTQ files from different samples according to barcode sequences given in a samplesheet.
	
	Mandatory parameters:
	  -sheet <file>     Samplesheet TSV file (lane, project, sample, i7 barcode, i5 barcode).
	  -in1 <filelist>   Input FASTQ forward file(s).
	  -in2 <filelist>   Input FASTQ reverse file(s).
	
	Optional parameters:
	  -out <string>     Output directory.
	                    Default value: 'Unaligned'
	  -mms <int>        Maximum tolerated mismatches for single barcode read.
	                    Default value: '1'
	  -mmd <int>        Maximum tolerated mismatches for double barcode (per barcode read).
	                    Default value: '2'
	  -rev2             Use reverse complement of second barcode sequence.
	                    Default value: 'false'
	  -summary <string> Summary file name (created in output folder). If unset, summary is written to STDOUT.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastqDemultiplex changelog
	FastqDemultiplex 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)