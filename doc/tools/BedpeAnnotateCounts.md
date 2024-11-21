### BedpeAnnotateCounts tool help
	BedpeAnnotateCounts (2024_08-110-g317f43b9)
	
	Annotates a BEDPE file with NGSD count information of zipped BEDPE flat files.
	
	Mandatory parameters:
	  -in <file>                  Input BEDPE file.
	  -out <file>                 Output BEDPE file.
	  -ann_folder <file>          Input folder containing NGSD count flat files.
	  -processing_system <string> Processing system short name of the processed sample
	
	Special parameters:
	  --help                      Shows this help and exits.
	  --version                   Prints version and exits.
	  --changelog                 Prints changeloge and exits.
	  --tdx                       Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]           Settings override file (no other settings files are used).
	
### BedpeAnnotateCounts changelog
	BedpeAnnotateCounts 2024_08-110-g317f43b9
	
	2022-02-11 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)