### VcfFilter tool help
	VcfFilter (2022_11-75-gf99b2041)
	
	Filters a VCF based on the given criteria.
	
	Missing annotation in the SAMPLE filter are treated as passing the filter.
	INFO flags (i.e. entries without value) are ignored, i.e. they cannot be filtered.
	
	Optional parameters:
	  -in <file>               Input VCF file. If unset, reads from STDIN.
	                           Default value: ''
	  -out <file>              Output VCF list. If unset, writes to STDOUT.
	                           Default value: ''
	  -reg <string>            Region of interest in BED format, or comma-separated list of region, e.g. 'chr1:454540-454678,chr2:473457-4734990'.
	                           Default value: ''
	  -remove_invalid          Removes invalid variant, i.e. invalid position of ref/alt.
	                           Default value: 'false'
	  -variant_type <string>   Filters by variant type. Possible types are: 'snp','indel','multi-allelic','other'.
	                           Default value: ''
	  -id <string>             Filter by ID column (regular expression).
	                           Default value: ''
	  -qual <float>            Filter by QUAL column (minimum).
	                           Default value: '0'
	  -filter <string>         Filter by FILTER column - keep matches (regular expression).
	                           Default value: ''
	  -filter_exclude <string> Filter by FILTER column - exclude matches (regular expression).
	                           Default value: ''
	  -filter_empty            Removes entries with non-empty FILTER column.
	                           Default value: 'false'
	  -info <string>           Filter by INFO column entries - use ';' as separator for several filters, e.g. 'DP > 5;AO > 2' (spaces are important).
	Valid operations are '>','>=','=','!=','<=','<','is','not','contains'.
	                           Default value: ''
	  -sample <string>         Filter by sample-specific entries - use ';' as separator for several filters, e.g. 'GT is 1/1' (spaces are important).
	Valid operations are '>','>=','=','!=','<=','<','is','not','contains'.
	                           Default value: ''
	  -sample_one_match        If set, a line will pass if one sample passes all filters (default behaviour is that all samples have to pass all filters).
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfFilter changelog
	VcfFilter 2022_11-75-gf99b2041
	
	2018-10-31 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)