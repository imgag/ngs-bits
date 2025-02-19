### NGSDExportStudyGHGA tool help
	NGSDExportStudyGHGA (2025_01-25-g1d2b52ea)
	
	Exports meta data of a study from NGSD to a JSON format for import into GHGA.
	
	Mandatory parameters:
	  -samples <file>     TSV file with pseudonym, SAP ID and processed sample ID (and optional sample folder)
	  -data <file>        JSON file with data that is not contained in NGSD.
	  -out <file>         Output JSON file.
	
	Optional parameters:
	  -include_bam        Add BAM files to output.
	                      Default value: 'false'
	  -include_vcf        Add VCF files to output.
	                      Default value: 'false'
	  -use_sample_folder  Use file names from sample folder provided in '-samples'.
	                      Default value: 'false'
	  -group_analyses     Combine all samples from one patient into a combined analysis (e. g. for tumor-normal).
	                      Default value: 'false'
	  -test               Test mode: uses the test NGSD
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]   Settings override file (no other settings files are used).
	
### NGSDExportStudyGHGA changelog
	NGSDExportStudyGHGA 2025_01-25-g1d2b52ea
	
	2025-02-07 Added option to combine analyses.
	2025-02-05 Added option to read files from sample folder.
	2024-09-11 Updated schema to version 2.0.0.
	2023-01-31 Initial implementation (version 0.9.0 of schema).
[back to ngs-bits](https://github.com/imgag/ngs-bits)