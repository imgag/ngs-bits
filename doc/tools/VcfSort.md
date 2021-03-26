### VcfSort tool help
	VcfSort (2021_03-23-g5c26fea8)
	
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
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfSort changelog
	VcfSort 2021_03-23-g5c26fea8
	
	2020-08-12 Added parameter '-compression_level' for compression level of output vcf files.
[back to ngs-bits](https://github.com/imgag/ngs-bits)