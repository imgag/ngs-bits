### VcfToTsv tool help
	VcfToTsv (2022_10-83-g62451d12)
	
	Converts a VCF file to a tab-separated text file.
	
	Multi-allelic variants are supported. All alternative sequences are stored as a comma-seperated list.
	Multi-sample VCFs are supported. For every combination of FORMAT and SAMPLE a seperate column is generated and named in the following way: <SAMPLEID>_<FORMATID>_<format>.
	
	Optional parameters:
	  -in <file>   Input variant list in VCF format. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output variant list in TSV format. If unset, writes to STDOUT.
	               Default value: ''
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfToTsv changelog
	VcfToTsv 2022_10-83-g62451d12
	
	2022-11-03 Changed output variant style from GSvar to VCF.
	2022-09-07 Added support for streaming (STDIN > STDOUT).
	2020-08-07 Multi-allelic and multi-sample VCFs are supported.
[back to ngs-bits](https://github.com/imgag/ngs-bits)