### BedAnnotateGC tool help
	BedAnnotateGC (2024_08-113-g94a3b440)
	
	Annotates GC content fraction to regions in a BED file.
	
	Optional parameters:
	  -in <file>        Input BED file. If unset, reads from STDIN.
	                    Default value: ''
	  -out <file>       Output BED file. If unset, writes to STDOUT.
	                    Default value: ''
	  -ref <file>       Reference genome FASTA file. If unset, 'reference_genome' from the 'settings.ini' file is used.
	                    Default value: ''
	  -extend <int>     Bases to extend around the input region for calculating the GC content.
	                    Default value: '0'
	  -clear            Clear all annotations present in the input file.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedAnnotateGC changelog
	BedAnnotateGC 2024_08-113-g94a3b440
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)