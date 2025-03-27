### FastqExtractBarcode tool help
	FastqExtractBarcode (2024_08-110-g317f43b9)
	
	Cuts bases from the beginning of reads and stores them in an additional fastq.
	
	Mandatory parameters:
	  -in <file>               input fastq file1.
	  -out_main <string>       output filename for main fastq.
	
	Optional parameters:
	  -out_index <string>      output filename for index fastq.
	                           Default value: 'index.fastq.gz'
	  -cut <int>               number of bases from the beginning of reads to use as barcodes.
	                           Default value: '0'
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### FastqExtractBarcode changelog
	FastqExtractBarcode 2024_08-110-g317f43b9
	
	2020-07-15 Added 'compression_level' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)