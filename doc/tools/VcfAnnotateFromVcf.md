### VcfAnnotateFromVcf tool help
	VcfAnnotateFromVcf (2024_08-110-g317f43b9)
	
	Annotates a VCF file with data from one or more source VCF files.
	
	NOTICE: the parameter '-existence_only' cannot be used together with '-config_file', '-info_keys' or '-id_column'.
	
	Optional parameters:
	  -in <file>                   Input VCF(.GZ) file that is annotated. If unset, reads from STDIN.
	                               Default value: ''
	  -out <file>                  Output VCF file. If unset, writes to STDOUT.
	                               Default value: ''
	  -config_file <file>          TSV file for annotation from multiple source files. For each source file, these tab-separated columns have to be given: source file name, prefix, INFO keys, ID column.
	                               Default value: ''
	  -source <file>               Tabix indexed VCF.GZ file that is the source of the annotated data.
	                               Default value: ''
	  -info_keys <string>          INFO key(s) in 'source' that should be annotated (Multiple keys are be separated by ',', optional keys can be renamed using this syntax: 'original_key=new_key').
	                               Default value: ''
	  -id_column <string>          ID column in 'source' (must be 'ID'). If unset, the ID column is not annotated. Alternative output name can be specified by using 'ID=new_name'.
	                               Default value: ''
	  -prefix <string>             Prefix added to all annotations in the output VCF file.
	                               Default value: ''
	  -allow_missing_header        If set the execution is not aborted if a INFO header is missing in the source file.
	                               Default value: 'false'
	  -existence_only              Only annotate if variant exists in source.
	                               Default value: 'false'
	  -existence_key_name <string> Defines the INFO key name.
	                               Default value: 'EXISTS_IN_SOURCE'
	  -threads <int>               The number of threads used to process VCF lines.
	                               Default value: '1'
	  -block_size <int>            Number of lines processed in one chunk.
	                               Default value: '10000'
	  -prefetch <int>              Maximum number of chunks that may be pre-fetched into memory.
	                               Default value: '64'
	  -debug                       Enables debug output (use only with one thread).
	                               Default value: 'false'
	
	Special parameters:
	  --help                       Shows this help and exits.
	  --version                    Prints version and exits.
	  --changelog                  Prints changeloge and exits.
	  --tdx                        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]            Settings override file (no other settings files are used).
	
### VcfAnnotateFromVcf changelog
	VcfAnnotateFromVcf 2024_08-110-g317f43b9
	
	2024-05-06 Added option to annotate the existence of variants in the source file
	2022-07-08 Usability: changed parameter names and updated documentation.
	2022-02-24 Refactoring and change to event-driven implementation (improved scaling with many threads)
	2021-09-20 Prefetch only part of input file (to save memory).
	2020-04-11 Added multithread support.
	2019-08-19 Added support for multiple annotations files through config file.
	2019-08-14 Added VCF.GZ support.
	2019-08-13 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)