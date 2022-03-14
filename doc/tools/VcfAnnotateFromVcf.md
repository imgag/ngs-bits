### VcfAnnotateFromVcf tool help
	VcfAnnotateFromVcf (2021_12-126-g84edc586)
	
	Annotates the INFO column of a VCF with data from another VCF file (or multiple VCF files if config file is provided).
	
	Optional parameters:
	  -in <file>              Input VCF(.GZ) file. If unset, reads from STDIN.
	                          Default value: ''
	  -out <file>             Output VCF list. If unset, writes to STDOUT.
	                          Default value: ''
	  -config_file <file>     TSV file containing the annotation file path, the prefix, the INFO ids and the id column for multiple annotations.
	                          Default value: ''
	  -annotation_file <file> Tabix indexed VCF.GZ file used for annotation.
	                          Default value: ''
	  -info_ids <string>      INFO id(s) in annotation VCF file (Multiple ids can be separated by ',', optional new id names in output file can be added by '=': original_id=new_id).
	                          Default value: ''
	  -id_column <string>     Name of the ID column in annotation file. (If  it will be ignored in output file, alternative output name can be specified by old_id_column_name=new_name
	                          Default value: ''
	  -id_prefix <string>     Prefix for INFO id(s) in output VCF file.
	                          Default value: ''
	  -allow_missing_header   If set the execution is not aborted if a INFO header is missing in annotation file.
	                          Default value: 'false'
	  -threads <int>          The number of threads used to process VCF lines.
	                          Default value: '1'
	  -block_size <int>       Number of lines processed in one chunk.
	                          Default value: '10000'
	  -prefetch <int>         Maximum number of chunks that may be pre-fetched into memory.
	                          Default value: '64'
	  -debug                  Enables debug output (use only with one thread).
	                          Default value: 'false'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfAnnotateFromVcf changelog
	VcfAnnotateFromVcf 2021_12-126-g84edc586
	
	2022-02-24 Refactoring and change to event-driven implementation (improved scaling with many threads)
	2021-09-20 Prefetch only part of input file (to save memory).
	2020-04-11 Added multithread support.
	2019-08-19 Added support for multiple annotations files through config file.
	2019-08-14 Added VCF.GZ support.
	2019-08-13 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)