### BedSort tool help
	BedSort (2020_03-184-g27235379)
	
	Sort the regions in a BED file.
	
	Optional parameters:
	  -in <file>   Input BED file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output BED file. If unset, writes to STDOUT.
	               Default value: ''
	  -with_name   Uses name column (i.e. the 4th column) to sort if chr/start/end are equal.
	               Default value: 'false'
	  -uniq        If set, entries with the same chr/start/end are removed after sorting.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### BedSort changelog
	BedSort 2020_03-184-g27235379
	
	2020-05-18 Added 'with_name' flag.
[back to ngs-bits](https://github.com/imgag/ngs-bits)