### ExportcBioportal tool help
	ExportcBioportal (2023_06-98-g044e3ed3)
	
	Converts a FASTQ file to FASTA format.
	
	Mandatory parameters:
	  -samples <file>        Input TSV file with samples (tumor, normal, rna) to be exported and their clinical data.
	  -study_data <file>     Input TSV file with Infos about the study that should be created.
	  -attribute_data <file> Input TSV file with Infos about the sample attributes that should be contained in the study.
	  -out <string>          Output folder that will contain all files for the cBioPortal study.
	
	Optional parameters:
	  -test                  Uses the test database instead of on the production database.
	                         Default value: 'false'
	
	Special parameters:
	  --help                 Shows this help and exits.
	  --version              Prints version and exits.
	  --changelog            Prints changeloge and exits.
	  --tdx                  Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### ExportcBioportal changelog
	ExportcBioportal 2023_06-98-g044e3ed3
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)