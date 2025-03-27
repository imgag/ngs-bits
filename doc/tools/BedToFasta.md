### BedToFasta tool help
	BedToFasta (2024_08-113-g94a3b440)
	
	Converts a BED file to a FASTA file.
	
	Mandatory parameters:
	  -in <file>        Input BED file.
	
	Optional parameters:
	  -out <file>       Output FASTA file. If unset, writes to STDOUT.
	                    Default value: ''
	  -ref <file>       Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedToFasta changelog
	BedToFasta 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)