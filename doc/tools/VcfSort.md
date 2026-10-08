### VcfSort tool help
	VcfSort (2026_06-46-g72aab0308)
	
	Sorts variant lists according to chromosomal position.
	
	Mandatory parameters:
	  -in <file>               Input variant list in VCF format.
	  -out <file>              Output variant list in VCF or VCF.GZ format.
	
	Optional parameters:
	  -compression_level <int> Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.
	                           Default value: '10'
	  -remove_unused_contigs   Remove comment lines of contigs, i.e. chromosomes, that are not used in the output VCF.
	                           Default value: 'false'
	  -split_chrs              Mode with reduced memory consumption for large files. Sorts only one chromosome at a time into a tmp file and merges all tmp files at the end.
	                           Default value: 'false'
	  -debug                   Enable debug output to STDOUT.
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### VcfSort changelog
	VcfSort 2026_06-46-g72aab0308
	
	2026-06-23 Added parameter '-split_chrs'. Removed parameters 'qual' and 'fai'.
	2022-12-08 Added parameter '-remove_unused_contigs'.
	2020-08-12 Added parameter '-compression_level' for compression level of output VCF files.
[back to ngs-bits](https://github.com/imgag/ngs-bits)