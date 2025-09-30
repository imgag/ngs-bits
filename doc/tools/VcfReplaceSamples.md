### VcfReplaceSamples tool help
	VcfReplaceSamples (2025_07-127-g60fc6b39)
	
	Replaces sample identifiers in the VCF header.
	
	Note: the sample ID matching is performed case-sensitive.
	
	Mandatory parameters:
	  -ids <string>            Comma-separated list of sample ID pairs in the format 'old1=new1,old2=new2,...'.
	
	Optional parameters:
	  -in <file>               Input variant list in VCF or VCF.GZ format. If unset, reads from STDIN.
	                           Default value: ''
	  -out <file>              Output variant list in VCF format. If unset, writes to STDOUT.
	                           Default value: ''
	  -compression_level <int> Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.
	                           Default value: '10'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### VcfReplaceSamples changelog
	VcfReplaceSamples 2025_07-127-g60fc6b39
	
	2025-08-27 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)