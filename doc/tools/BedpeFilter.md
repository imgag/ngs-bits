### BedpeFilter tool help
	BedpeFilter (2025_12-266-g396e1fe11)
	
	Filters a BEDPE file by region.
	
	Optional parameters:
	  -in <file>        Input BEDPE file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output BEDPE file. If unset, writes to STDOUT.
	                    Default value: ''
	  -bed <file>       BED file that is used as ROI. Only one of the SV breakpoint has to be in the target region!
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedpeFilter changelog
	BedpeFilter 2025_12-266-g396e1fe11
	
	2020-06-05 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)