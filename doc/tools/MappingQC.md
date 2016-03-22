### MappingQC tool help
	MappingQC (0.1-256-g371df50)
	
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
	  -txt            Writes TXT format instead of qcML.
	                  Default value: 'false'
	  -min_mapq <int> Minmum mapping quality to consider a read mapped.
	                  Default value: '1'
	  -3exons         Adds special QC terms estimating the sequencing error on reads from three exons.
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)