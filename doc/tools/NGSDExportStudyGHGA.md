### NGSDExportStudyGHGA tool help
	NGSDExportStudyGHGA (2024_08-36-g4fed1f49)
	
	Exports meta data of a study from NGSD to a JSON format for import into GHGA.
	
	Mandatory parameters:
	  -samples <file> TSV file with pseudonym, SAP ID and processed sample ID
	  -data <file>    JSON file with data that is not contained in NGSD.
	  -out <file>     Output JSON file.
	
	Optional parameters:
	  -include_vcf    Add VCF files to output.
	                  Default value: 'false'
	  -test           Test mode: uses the test NGSD
	                  Default value: 'false'
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDExportStudyGHGA changelog
	NGSDExportStudyGHGA 2024_08-36-g4fed1f49
	
	2024-09-11 Updated schema to version 2.0.0.
	2023-01-31 Initial implementation (version 0.9.0 of schema).
[back to ngs-bits](https://github.com/imgag/ngs-bits)