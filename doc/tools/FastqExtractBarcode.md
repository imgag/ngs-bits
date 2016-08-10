### FastqExtractBarcode tool help
	FastqExtractBarcode (0.1-420-g3536bb0)
	
	Cuts bases from the beginning of reads and stores them in an additional fastq.
	
	Mandatory parameters:
	  -in <file>          input fastq file1.
	  -out_main <string>  output filename for main fastq.
	
	Optional parameters:
	  -cut <int>          number of bases from the beginning of reads to use as barcodes.
	                      Default value: '0'
	  -out_index <string> output filename for index fastq.
	                      Default value: 'index.fastq.gz'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### FastqExtractBarcode changelog
	FastqExtractBarcode 0.1-420-g3536bb0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)