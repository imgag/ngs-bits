### BedToEpigen tool help
	BedToEpigen (2025_09-112-ga53ff6f3d)
	
	Converts a modkit BED file to a Epigen TSV file.
	
	Mandatory parameters:
	  -id_file <file>   Input CSV file containing Illumina CpG IDs.
	  -sample <string>  Sample name used in output file header.
	
	Optional parameters:
	  -in <file>        Input modkit (bgzipped) BED file. If unset, read from STDIN.
	                    Default value: ''
	  -out <file>       Output FASTA file. If unset, writes to STDOUT.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BedToEpigen changelog
	BedToEpigen 2025_09-112-ga53ff6f3d
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)