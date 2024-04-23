### BedpeAnnotateBreakpointDensity tool help
	BedpeAnnotateBreakpointDensity (2024_02-42-g36bb2635)
	
	Annotates a BEDPE file with breakpoint density.
	
	Mandatory parameters:
	  -density <file>     IGV density file containing break point density.
	
	Optional parameters:
	  -density_sys <file> Optional IGV density file containing break point density for a specific processing system.
	                      Default value: ''
	  -in <file>          Input BEDPE file. If unset, reads from STDIN.
	                      Default value: ''
	  -out <file>         Output BEDPE file. If unset, writes to STDOUT.
	                      Default value: ''
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedpeAnnotateBreakpointDensity changelog
	BedpeAnnotateBreakpointDensity 2024_02-42-g36bb2635
	
	2024-02-26 Added system-specific density.
	2022-02-23 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)