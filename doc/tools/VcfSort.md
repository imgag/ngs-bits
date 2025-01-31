### VcfSort tool help
	VcfSort (2024_08-110-g317f43b9)
	
	Sorts variant lists according to chromosomal position.
	
	Mandatory parameters:
	  -in <file>               Input variant list in VCF format.
	  -out <file>              Output variant list in VCF or VCF.GZ format.
	
	Optional parameters:
	  -qual                    Also sort according to variant quality. Ignored if 'fai' file is given.
	                           Default value: 'false'
	  -fai <file>              FAI file defining different chromosome order.
	                           Default value: ''
	  -compression_level <int> Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.
	                           Default value: '10'
	  -remove_unused_contigs   Remove comment lines of contigs, i.e. chromosomes, that are not used in the output VCF.
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### VcfSort changelog
	VcfSort 2024_08-110-g317f43b9
	
	2022-12-08 Added parameter '-remove_unused_contigs'.
	2020-08-12 Added parameter '-compression_level' for compression level of output VCF files.
[back to ngs-bits](https://github.com/imgag/ngs-bits)