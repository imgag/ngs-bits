### MappingQC tool help
	MappingQC (0.1-782-ge325449)
	
	Calculates QC metrics based on mapped NGS reads.
	
	Mandatory parameters:
	  -in <file>      Input BAM file.
	
	Optional parameters:
	  -out <file>     Output qcML file. If unset, writes to STDOUT.
	                  Default value: ''
	  -roi <file>     Input target region BED file (for panel, WES, etc.).
	                  Default value: ''
	  -wgs            WGS mode without target region. Genome information is taken from the BAM file.
	                  Default value: 'false'
	  -rna            RNA mode without target region. Genome information is taken from the BAM file.
	                  Default value: 'false'
	  -txt            Writes TXT format instead of qcML.
	                  Default value: 'false'
	  -min_mapq <int> Minmum mapping quality to consider a read mapped.
	                  Default value: '1'
	  -3exons         Adds special QC terms estimating the sequencing error on reads from three exons.
	                  Default value: 'false'
	  -no_cont        Disables sample contamination calculation, e.g. for tumor or non-human samples.
	                  Default value: 'false'
	  -debug          Enables verbose debug outout.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### MappingQC changelog
	MappingQC 0.1-782-ge325449
	
	2016-12-20 Added support for spliced RNA reads (relevant e.g. for insert size)
[back to ngs-bits](https://github.com/imgag/ngs-bits)