### VcfToTsv tool help
	VcfToTsv (2018_11-7-g60f117b)
	
	Converts a VCF file to a tab-separated text file.
	
	Multi-allelic variants are not supported. Use VcfBreakMulti to split multi-allelic variants into several lines.
	Multi-sample VCFs are not supported. Use VcfExtractSamples to split them to one VCF per sample.
	
	Mandatory parameters:
	  -in <file>   Input variant list in VCF format.
	  -out <file>  Output variant list in TSV format.
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfToTsv changelog
	VcfToTsv 2018_11-7-g60f117b
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)