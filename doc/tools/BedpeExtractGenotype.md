### BedpeExtractGenotype tool help
	BedpeExtractGenotype (2024_08-110-g317f43b9)
	
	Extracts the phased genotype into seperate column.
	
	Optional parameters:
	  -in <file>         Input BEDPE file. If unset, reads from STDIN.
	                     Default value: ''
	  -out <file>        Output BEDPE file. If unset, writes to STDOUT.
	                     Default value: ''
	  -include_unphased  Also annotate genotype of unphased SVs.
	                     Default value: 'false'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]  Settings override file (no other settings files are used).
	
### BedpeExtractGenotype changelog
	BedpeExtractGenotype 2024_08-110-g317f43b9
	
	2023-10-04 Added parameter to also annotate unphased genotype.
	2023-09-22 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)