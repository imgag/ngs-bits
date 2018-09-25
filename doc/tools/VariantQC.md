### VariantQC tool help
	VariantQC (2018_06-35-ga1d5548)
	
	Calculates QC metrics on variant lists.
	
	Mandatory parameters:
	  -in <file>      Input variant list in VCF format.
	
	Optional parameters:
	  -ignore_filter  Ignore filter entries, i.e. consider variants that did not pass filters.
	                  Default value: 'false'
	  -out <file>     Output qcML file. If unset, writes to STDOUT.
	                  Default value: ''
	  -txt            Writes TXT format instead of qcML.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantQC changelog
	VariantQC 2018_06-35-ga1d5548
	
	2018-09-12 Now supports VEP CSQ annotations (no longer support SnpEff ANN annotations).
	2017-01-05 Added 'ignore_filter' flag.
[back to ngs-bits](https://github.com/imgag/ngs-bits)