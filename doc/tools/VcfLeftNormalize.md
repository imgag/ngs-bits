### VcfLeftNormalize tool help
	VcfLeftNormalize (2023_11-133-g87eceb58)
	
	Normalizes all variants and shifts indels to the left in a VCF file.
	
	Note: Multi-allelic variants, SNVs, MNPs, complex InDels and invalid variants are skipped!
	
	Optional parameters:
	  -in <file>               Input VCF or VCF.GZ file. If unset, reads from STDIN.
	                           Default value: ''
	  -out <file>              Output VCF or VCF or VCF.GZ file. If unset, writes to STDOUT.
	                           Default value: ''
	  -ref <file>              Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                           Default value: ''
	  -compression_level <int> Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.
	                           Default value: '10'
	  -stream                  Allows to stream the input and output VCF without loading the whole file into memory. Only supported with uncompressed VCF files.
	                           Default value: 'false'
	  -right                   Right-normalize VCF instead of left-normalizing it.
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfLeftNormalize changelog
	VcfLeftNormalize 2023_11-133-g87eceb58
	
	2020-08-12 Added parameter '-compression_level' for compression level of output VCF files.
	2016-06-24 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)