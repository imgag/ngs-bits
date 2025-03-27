### FastqCheckUMI tool help
	FastqCheckUMI (2024_08-113-g94a3b440)
	
	Returns the UMI info of a FastQ file on STDOUT.
	
	Mandatory parameters:
	  -in <file>        Input FASTQ file.
	
	Optional parameters:
	  -out <file>       Output file containing the result string. If unset, writes to STDOUT.
	                    Default value: ''
	  -lines <int>      Number of lines which should be checked. (default: 10)
	                    Default value: '10'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### FastqCheckUMI changelog
	FastqCheckUMI 2024_08-113-g94a3b440
	
	2023-10-05 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)