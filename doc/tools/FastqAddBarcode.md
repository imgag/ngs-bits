### FastqAddBarcode tool help
	FastqAddBarcode (2024_08-110-g317f43b9)
	
	Adds barcodes from separate FASTQ file to read headers.
	
	Mandatory parameters:
	  -in1 <filelist>          Input FASTQ file 1.
	  -in2 <filelist>          Input FASTQ file 2.
	  -in_barcode <filelist>   Input barcode file.
	  -out1 <file>             Output filename for read 1 FASTQ.
	  -out2 <file>             Output filename for read 2 FASTQ.
	
	Optional parameters:
	  -compression_level <int> Output FASTQ compression level from 1 (fastest) to 9 (best compression).
	                           Default value: '1'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### FastqAddBarcode changelog
	FastqAddBarcode 2024_08-110-g317f43b9
	
	2020-07-15 Added 'compression_level' parameter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)