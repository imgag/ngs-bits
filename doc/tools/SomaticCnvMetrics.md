### SomaticCnvMetrics tool help
	SomaticCnvMetrics (2022_04-62-gc5232248)
	
	Calculate somatic CNV metrics based on CNV file.
	
	Mandatory parameters:
	  -in <file>    Input somatic ClinCNV file in TSV format.
	
	Optional parameters:
	  -out <file>   Output qcML file. Writes to stdout otherwise.
	                Default value: ''
	  -build <enum> Reference genome to be used.
	                Default value: 'hg38'
	                Valid: 'hg19,hg38'
	  -tid <string> Tumor processed sample ID (for retrieving CNVs flagged as artefact from NGSD.)
	                Default value: ''
	  -nid <string> Normal processed sample ID (for retrieving CNVs flagged as artefact from NGSD.)
	                Default value: ''
	
	Special parameters:
	  --help        Shows this help and exits.
	  --version     Prints version and exits.
	  --changelog   Prints changeloge and exits.
	  --tdx         Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### SomaticCnvMetrics changelog
	SomaticCnvMetrics 2022_04-62-gc5232248
	
	2021-05-06 Initial version of the tool to calculate HRD score.
[back to ngs-bits](https://github.com/imgag/ngs-bits)