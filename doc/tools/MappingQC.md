### MappingQC tool help
	MappingQC (0.1-52-g9f9161f)
	
	Calculates QC metrics based on mapped NGS reads.
	
	Mandatory parameters:
	  -in <file>  Input BAM file.
	
	Optional parameters:
	  -out <file> Output qcML file. If unset, writes to STDOUT.
	              Default value: ''
	  -roi <file> Input target region BED file.
	              Default value: ''
	  -wgs <enum> Calculates statistics for WGS instead of a specific target region.
	              Default value: ''
	              Valid: ',hg19'
	  -txt        Writes TXT format instead of qcML.
	              Default value: 'false'
	  -3exons     Adds special QC terms estimating the sequencing error on reads from three exons.
	              Default value: 'false'
	
	Special parameters:
	  --help      Shows this help and exits.
	  --version   Prints version and exits.
	  --tdx       Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'.
	
[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)