### TrioMendelianErrors tool help
	TrioMendelianErrors (2025_01-25-g1d2b52ea)
	
	Determines mendelian error rate from a trio VCF.
	
	Mandatory parameters:
	  -vcf <file>       Multi-sample VCF or VCF.GZ file.
	  -c <string>       Sample name of child in VCF.
	  -f <string>       Sample name of father in VCF.
	  -m <string>       Sample name of mother in VCF.
	
	Optional parameters:
	  -out <file>       Output text file. If unset, writes to STDOUT.
	                    Default value: ''
	  -min_dp <int>     Minimum depth in each sample.
	                    Default value: '0'
	  -min_qual <float> Minimum QUAL of variants.
	                    Default value: '0'
	  -debug            Enable debug output
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### TrioMendelianErrors changelog
	TrioMendelianErrors 2025_01-25-g1d2b52ea
	
	2025-02-18 Initial version of the tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)