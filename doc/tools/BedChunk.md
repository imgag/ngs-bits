### BedChunk tool help
	BedChunk (2023_06-81-ga3af580a)
	
	Splits all regions to chunks of an approximate desired size.
	
	Mandatory parameters:
	  -n <int>     The desired chunk size. Note: Not all chunks will have this size. Regions are split to chunks that are closest to the desired size.
	
	Optional parameters:
	  -in <file>   Input BED file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output BED file. If unset, writes to STDOUT.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedChunk changelog
	BedChunk 2023_06-81-ga3af580a
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)