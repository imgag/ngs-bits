### VcfAnnotateFromVcf tool help
	VcfAnnotateFromVcf (2020_03-72-ga487e6e9)
	
	Annotates the INFO column of a VCF with data from another VCF file (or multiple VCF files if config file is provided).
	
	Optional parameters:
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
	  -allow_missing_header   If set the execution is not aborted if a INFO header is missing in annotation file
	                          Default value: 'false'
	  -in <file>              Input VCF(.GZ) file. If unset, reads from STDIN.
	                          Default value: ''
	  -out <file>             Output VCF list. If unset, writes to STDOUT.
	                          Default value: ''
	  -max_job_number <int>   The max Number of jobs
	                          Default value: '1000'
	  -threads <int>          The number of threads used to read, process and write files.
	                          Default value: '1'
	  -block_size <int>       Number of lines in one block
	                          Default value: '5000'
	
	Special parameters:
	  --help                  Shows this help and exits.
	  --version               Prints version and exits.
	  --changelog             Prints changeloge and exits.
	  --tdx                   Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfAnnotateFromVcf changelog
	VcfAnnotateFromVcf 2020_03-72-ga487e6e9
	
	2020-04-11 Added multithread support by Julian Fratte.
	2019-08-19 Added support for multiple annotations files through config file.
	2019-08-14 Added VCF.GZ support.
	2019-08-13 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)