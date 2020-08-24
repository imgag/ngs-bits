### VariantFilterRegions tool help
	VariantFilterRegions (2020_03-260-ge35d12de)
	
	Filter a variant list based on a target region.
	
	Mandatory parameters:
	  -in <file>     Input variant list. In vcf (default) or gsvar format.
	  -out <file>    Output variant list.
	
	Optional parameters:
	  -reg <file>    Input target region in BED format.
	                 Default value: ''
	  -r <string>    Single target region in the format chr17:41194312-41279500.
	                 Default value: ''
	  -mark <string> If set, instead of removing variants, they are marked with the given flag in the 'filter' column.
	                 Default value: ''
	  -inv           Inverts the filter, i.e. variants inside the region are removed/marked.
	                 Default value: 'false'
	  -mode <enum>   Mode (input format).
	                 Default value: 'vcf'
	                 Valid: 'vcf,gsvar'
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantFilterRegions changelog
	VariantFilterRegions 2020_03-260-ge35d12de
	
	2018-01-23 Added parameter '-inv' and made parameter '-mark' a string parameter to allow custom annotations names.
	2017-01-04 Added parameter '-mark' for flagging variants instead of filtering them out.
	2016-06-10 Added single target region parameter '-r'.
[back to ngs-bits](https://github.com/imgag/ngs-bits)