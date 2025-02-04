### VcfSubtract tool help
	VcfSubtract (2024_11-59-ge0a7288e)
	
	Substracts the variants in a VCF from a second VCF.
	
	Mandatory parameters:
	  -in2 <file>       Variants in VCF format that are remove from 'in'
	
	Optional parameters:
	  -in <file>        Input VCF file from which the variants of 'in2' are substracted.
	                    Default value: ''
	  -out <file>       Output VCF file with variants from 'in2' removed from 'in'.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfSubtract changelog
	VcfSubtract 2024_11-59-ge0a7288e
	
	2023-02-06 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)