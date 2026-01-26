### BedpeAnnotateBreakpointDensity tool help
	BedpeAnnotateBreakpointDensity (2025_03-80-g74f31dd7)
	
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
	  --settings [file]   Settings override file (no other settings files are used).
	
### BedpeAnnotateBreakpointDensity changelog
	BedpeAnnotateBreakpointDensity 2025_03-80-g74f31dd7
	
	2025-03-25 Fixed crashes in std::max_element when QVector is empty.
	2024-02-26 Added system-specific density.
	2022-02-23 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)