### VcfToTsv tool help
	VcfToTsv (2020_03-260-ge35d12de)
	
	Converts a VCF file to a tab-separated text file.
	
	Multi-allelic variants are supported. All alternative sequences are stored as a comma-seperated list.
	Multi-sample VCFs are supported. For every combination of FORMAT and SAMPLE a seperate column is generated and named in the following way: <SAMPLEID>_<FORMATID>_<format>.
	
	Mandatory parameters:
	  -in <file>   Input variant list in VCF format.
	  -out <file>  Output variant list in TSV format.
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfToTsv changelog
	VcfToTsv 2020_03-260-ge35d12de
	
	2020-08-07 Multi-allelic and Multi-sample VCFs are supported.
[back to ngs-bits](https://github.com/imgag/ngs-bits)