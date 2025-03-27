### BedShrink tool help
	BedShrink (2024_08-113-g94a3b440)
	
	Shrinks the regions in a BED file.
	
	Mandatory parameters:
	  -n <int>          The number of bases to shrink (on both sides of each region).
	
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
	
### BedShrink changelog
	BedShrink 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)