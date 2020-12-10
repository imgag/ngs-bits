### MappingQC tool help
	MappingQC (2020_09-79-gc6314b21)
	
	Calculates QC metrics based on mapped NGS reads.
	
	Mandatory parameters:
	  -in <file>      Input BAM/CRAM file.
	
	Optional parameters:
	  -out <file>     Output qcML file. If unset, writes to STDOUT.
	                  Default value: ''
	  -roi <file>     Input target region BED file (for panel, WES, etc.).
	                  Default value: ''
	  -wgs            WGS mode without target region. Genome information is taken from the BAM/CRAM file.
	                  Default value: 'false'
	  -rna            RNA mode without target region. Genome information is taken from the BAM/CRAM file.
	                  Default value: 'false'
	  -txt            Writes TXT format instead of qcML.
	                  Default value: 'false'
	  -min_mapq <int> Minmum mapping quality to consider a read mapped.
	                  Default value: '1'
	  -no_cont        Disables sample contamination calculation, e.g. for tumor or non-human samples.
	                  Default value: 'false'
	  -debug          Enables verbose debug outout.
	                  Default value: 'false'
	  -build <enum>   Genome build used to generate the input (needed for contamination only).
	                  Default value: 'hg19'
	                  Valid: 'hg19,hg38'
	  -ref <string>   Reference genome for CRAM support (mandatory if CRAM is used).
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### MappingQC changelog
	MappingQC 2020_09-79-gc6314b21
	
	2020-11-27 Added CRAM support.
	2018-07-11 Added build switch for hg38 support.
	2018-03-29 Removed '3exons' flag.
	2016-12-20 Added support for spliced RNA reads (relevant e.g. for insert size)
[back to ngs-bits](https://github.com/imgag/ngs-bits)