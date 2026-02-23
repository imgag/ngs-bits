### NGSDImportCSpec tool help
	NGSDImportCSpec (2025_12-104-g15d04bf0)
	
	Import genes with special interpretation guidelines from CSpect.
	
	Mandatory parameters:
	  -in <file>        CSpect data JSON downloaded from 'https://cspec.genome.network/cspec/SequenceVariantInterpretation/id?detail=high&fields=ld.RuleSet,ldFor.Organization,entContent.states,entContent.legacyFullySuperseded,entContent.legacyReplaced,entId,ldhId,entContent.title&pgSize=1000'
	
	Optional parameters:
	  -test             Uses the test database instead of on the production database.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### NGSDImportCSpec changelog
	NGSDImportCSpec 2025_12-104-g15d04bf0
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)