### NGSDTransferReportConfig tool help
	NGSDTransferReportConfig (2025_12-175-gc2187ef59)
	
	Transfers (germline) Report Configuration from one sample to another.
	
	Mandatory parameters:
	  -source_ps <string> Processed sample name from which the ReportConfig is taken.
	  -target_ps <string> Processed sample name to which the ReportConfig is transferred to.
	
	Optional parameters:
	  -force              Transfer report even if some variants aren't present in the target sample (Missing variants will be written into the `report_configuration_failed_transfer` table.)
	                      Default value: 'false'
	  -test               Uses the test database instead of on the production database.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]   Settings override file (no other settings files are used).
	
### NGSDTransferReportConfig changelog
	NGSDTransferReportConfig 2025_12-175-gc2187ef59
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)