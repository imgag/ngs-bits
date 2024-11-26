### BedSubtract tool help
	BedSubtract (2024_08-113-g94a3b440)
	
	Subtracts the regions in one BED file from another.
	
	Mandatory parameters:
	  -in2 <file>       Input BED file which is subtracted from 'in'.
	
	Optional parameters:
	  -in <file>        Input BED file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output BED file. If unset, writes to STDOUT.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedSubtract changelog
	BedSubtract 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)