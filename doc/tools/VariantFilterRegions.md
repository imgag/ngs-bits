### VariantFilterRegions tool help
	VariantFilterRegions (2021_03-23-g5c26fea8)
	
	Filter a variant list based on a target region.
	
	Mandatory parameters:
	  -in <file>               Input variant list. In VCF (default) or GSvar format.
	  -out <file>              Output variant list (same format as 'in').
	
	Optional parameters:
	  -reg <file>              Input target region in BED format.
	                           Default value: ''
	  -r <string>              Single target region in the format chr17:41194312-41279500.
	                           Default value: ''
	  -mark <string>           If set, instead of removing variants, they are marked with the given flag in the 'filter' column.
	                           Default value: ''
	  -inv                     Inverts the filter, i.e. variants inside the region are removed/marked.
	                           Default value: 'false'
	  -mode <enum>             Mode (input format).
	                           Default value: 'vcf'
	                           Valid: 'vcf,gsvar'
	  -compression_level <int> Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.
	                           Default value: '10'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VariantFilterRegions changelog
	VariantFilterRegions 2021_03-23-g5c26fea8
	
	2020-08-12 Added parameter '-compression_level' for compression level of output vcf files.
	2018-01-23 Added parameter '-inv' and made parameter '-mark' a string parameter to allow custom annotations names.
	2017-01-04 Added parameter '-mark' for flagging variants instead of filtering them out.
	2016-06-10 Added single target region parameter '-r'.
[back to ngs-bits](https://github.com/imgag/ngs-bits)