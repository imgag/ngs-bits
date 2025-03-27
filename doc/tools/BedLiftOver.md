### BedLiftOver tool help
	BedLiftOver (2024_08-110-g317f43b9)
	
	Lift-over of regions in a BED file to a different genome build.
	
	Mandatory parameters:
	  -in <file>           Input BED file with the regions to lift.
	  -out <file>          The file where the lifted regions will be written to.
	
	Optional parameters:
	  -unmapped <file>     The file where the unmappable regions will be written to.
	                       Default value: ''
	  -chain <string>      Input Chain file in .chain/.chain.gz format or "hg19_hg38" / "hg38_hg19" to read from settings file.
	                       Default value: 'hg19_hg38'
	  -max_deletion <int>  Allowed percentage of deleted/unmapped bases in each region.
	                       Default value: '5'
	  -max_increase <int>  Allowed percentage size increase of a region.
	                       Default value: '10'
	  -remove_special_chr  Removes regions that are mapped to special chromosomes.
	                       Default value: 'false'
	
	Special parameters:
	  --help               Shows this help and exits.
	  --version            Prints version and exits.
	  --changelog          Prints changeloge and exits.
	  --tdx                Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]    Settings override file (no other settings files are used).
	
### BedLiftOver changelog
	BedLiftOver 2024_08-110-g317f43b9
	
	2022-02-14 First implementation
[back to ngs-bits](https://github.com/imgag/ngs-bits)