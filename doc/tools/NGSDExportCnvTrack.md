### NGSDExportCnvTrack tool help
	NGSDExportCnvTrack (2024_08-110-g317f43b9)
	
	Exports a IGV-conform CNV track for a processing system.
	
	Mandatory parameters:
	  -out <file>              Output IGV file.
	
	Optional parameters:
	  -system <string>         Processing system name filter (short name).
	                           Default value: ''
	  -min_dp <float>          Minimum depth of the processed sample.
	                           Default value: '0'
	  -max_cnvs <float>        Maximum number of CNVs per sample.
	                           Default value: '0'
	  -min_af <float>          Minimum allele frequency of output CNV ranges.
	                           Default value: '0.01'
	  -caller_version <string> Restrict output to callsets with this caller version.
	                           Default value: ''
	  -stats <file>            Statistics and logging output. If unset, writes to STDOUT
	                           Default value: ''
	  -test                    Uses the test database instead of on the production database.
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### NGSDExportCnvTrack changelog
	NGSDExportCnvTrack 2024_08-110-g317f43b9
	
	2019-10-21 First version
[back to ngs-bits](https://github.com/imgag/ngs-bits)