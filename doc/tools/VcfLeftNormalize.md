### VcfLeftNormalize tool help
	VcfLeftNormalize (2020_06-89-g5de737ff)
	
	Normalizes all variants and shifts indels to the left in a VCF file. Multi-allelic and complex variant are not changed!
	
	Optional parameters:
	  -in <file>               Input VCF file. If unset, reads from STDIN.
	                           Default value: ''
	  -out <file>              Output VCF or VCF or VCF.GZ file. If unset, writes to STDOUT.
	                           Default value: ''
	  -ref <file>              Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                           Default value: ''
	  -compression_level <int> Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.
	                           Default value: '0'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfLeftNormalize changelog
	VcfLeftNormalize 2020_06-89-g5de737ff
	
	2020-08-12 Added parameter '-compression_level' for compression level of output vcf files.
	2016-06-24 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)