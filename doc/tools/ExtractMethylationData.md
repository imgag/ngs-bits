### ExtractMethylationData tool help
	ExtractMethylationData (2024_08-110-g317f43b9)
	
	Extracts the methylation state for a given set of loci
	
	Mandatory parameters:
	  -in <file>              Tabix indexed BED.GZ file that contains the methylation info for each base (modkit).
	  -loci <file>            BED file containig position and strand of intrest
	
	Optional parameters:
	  -out <file>             Output BED file containing combined methylation info of provided loci. If unset, writes to STDOUT.
	                          Default value: ''
	  -ref <file>             Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                          Default value: ''
	  -add_methylation_types  Also report 5mC (m) and 5hmC (h) entries as separate columns
	                          Default value: 'false'
	  -skip_invalid_sites     Skip invalid CpG sites instead of aborting.
	                          Default value: 'false'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]       Settings override file (no other settings files are used).
	
### ExtractMethylationData changelog
	ExtractMethylationData 2024_08-110-g317f43b9
	
	2024-09-27 Added option to ignore invalid CpG sites.
	2024-07-18 Added option to add separate columns for 5mC/5hmC.
	2024-06-26 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)