### VcfStrip tool help
	VcfStrip (2024_08-113-g94a3b440)
	
	Removes unwanted information from a VCF file.
	
	Optional parameters:
	  -in <file>        Input VCF file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output VCF file. If unset, writes to STDOUT.
	                    Default value: ''
	  -info <string>    Comma-separated list of INFO entries to keep. If unset, all INFO entries are kept.
	                    Default value: ''
	  -format <string>  Comma-separated list of FORMAT entries to keep. If unset, all FORMAT entries are kept
	                    Default value: ''
	  -clear_info       Remove all INFO fields
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfStrip changelog
	VcfStrip 2024_08-113-g94a3b440
	
	2024-11-20 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)