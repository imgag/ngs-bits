### GenlabInfo tool help
	GenlabInfo (2025_07-127-g60fc6b39)
	
	Provide sample information from GenLAB.
	
	Mandatory parameters:
	  -ps <string>      Processed sample or TSV file with processed sample ids in the first column
	
	Optional parameters:
	  -info <string>    Infos that will be collected from Genlab.Comma seperated list of values. Supported: SAPID,PATID
	                    Default value: 'SAPID'
	  -out <string>     TSV file where the Genlab infos will be written to. stdout if emtpy.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### GenlabInfo changelog
	GenlabInfo 2025_07-127-g60fc6b39
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)