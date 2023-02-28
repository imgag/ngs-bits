### NGSDExportStudyGHGA tool help
	NGSDExportStudyGHGA (2022_12-82-g025eb99e)
	
	Exports meta data of a study from NGSD to a JSON format for import into GHGA.
	
	Mandatory parameters:
	  -data <file> JSON file with data that is not contained in NGSD.
	  -out <file>  Output JSON file.
	
	Optional parameters:
	  -test        Test mode: uses the test NGSD, does not calcualte size/checksum of BAMs, ...
	               Default value: 'false'
	
	Special parameters:
	  --help       Shows this help and exits.
	  --version    Prints version and exits.
	  --changelog  Prints changeloge and exits.
	  --tdx        Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDExportStudyGHGA changelog
	NGSDExportStudyGHGA 2022_12-82-g025eb99e
	
	2023-01-31 Initial implementation (version 0.9.0 of schema).
[back to ngs-bits](https://github.com/imgag/ngs-bits)