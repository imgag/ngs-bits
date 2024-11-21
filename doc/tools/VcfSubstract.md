### VcfSubstract tool help
	VcfSubstract (2024_08-113-g94a3b440)
	
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
	
### VcfSubstract changelog
	VcfSubstract 2024_08-113-g94a3b440
	
	2023-02-06 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)