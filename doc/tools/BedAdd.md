### BedAdd tool help
	BedAdd (2024_08-113-g94a3b440)
	
	Merges regions from several BED files.
	
	Mandatory parameters:
	  -in <filelist>    Input BED files.
	
	Optional parameters:
	  -out <file>       Output BED file. If unset, writes to STDOUT.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedAdd changelog
	BedAdd 2024_08-113-g94a3b440
	
	2018-04-03 Removed 'in2' argument and made 'in' a file list.
[back to ngs-bits](https://github.com/imgag/ngs-bits)