### MantaVcfFix tool help
	MantaVcfFix (2025_01-25-g1d2b52ea)
	
	Fixes issues in VCF of Manta SV calls.
	
	Removes invalid VCF lines containing empty REF entries.
	Removes duplicate SV calls from Manta VCFs.
	
	Mandatory parameters:
	  -in <file>        Input VCF file.
	  -out <file>       Output VCF file.
	
	Optional parameters:
	  -debug            Print verbose output to STDERR.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### MantaVcfFix changelog
	MantaVcfFix 2025_01-25-g1d2b52ea
	
	2025-02-04 added filtering.
	2024-10-31 initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)