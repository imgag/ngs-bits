### VcfCheck tool help
	VcfCheck (2018_06-59-g24102d3)
	
	Checks a VCF file for errors.
	
	Checks the input VCF file with SNVs and small InDels for errors and warnings.
	If the VEP-based CSQ annotation is present, it also checks that the Miso terms in the consequence field are valid.
	
	Optional parameters:
	  -in <file>   Input VCF file. If unset, reads from STDIN.
	               Default value: ''
	  -out <file>  Output file. If unset, writes to STDOUT.
	               Default value: ''
	  -lines <int> Number of lines to check in the VCF file (unlimited if 0)
	               Default value: '1000'
	  -ref <file>  Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	               Default value: ''
	  -info        Add general information about the input file to the output.
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfCheck changelog
	VcfCheck 2018_06-59-g24102d3
	
	2018-12-03 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)