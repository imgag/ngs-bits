### VariantQC tool help
	VariantQC (2024_08-110-g317f43b9)
	
	Calculates QC metrics on variant lists.
	
	Mandatory parameters:
	  -in <file>          Input variant list in VCF format.
	
	Optional parameters:
	  -ignore_filter      Ignore filter entries, i.e. consider variants that did not pass filters.
	                      Default value: 'false'
	  -out <file>         Output qcML file. If unset, writes to STDOUT.
	                      Default value: ''
	  -txt                Writes TXT format instead of qcML.
	                      Default value: 'false'
	  -long_read          Adds LongRead specific QC values (e.g. phasing information)
	                      Default value: 'false'
	  -phasing_bed <file> Output BED file containing phasing blocks with id. (requires parameter '-longread')
	                      Default value: ''
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]   Settings override file (no other settings files are used).
	
### VariantQC changelog
	VariantQC 2024_08-110-g317f43b9
	
	2023-09-21 Added parameter 'longread' to add longread specific QC values.
	2020-08-07 VCF files only as input format for variant list.
	2018-09-12 Now supports VEP CSQ annotations (no longer support SnpEff ANN annotations).
	2017-01-05 Added 'ignore_filter' flag.
[back to ngs-bits](https://github.com/imgag/ngs-bits)