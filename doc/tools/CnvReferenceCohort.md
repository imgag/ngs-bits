### CnvReferenceCohort tool help
	CnvReferenceCohort (2024_08-36-g4fed1f49)
	
	Create a reference cohort for CNV calling from a list of coverage profiles.
	
	This tool creates a reference cohort for CNV calling by analyzing the correlation between the main sample coverage profile and a list of reference coverage profiles.
	The coverage profiles of the samples that correlate best with the main sample are saved in the output TSV file.
	The TSV file contains the chromosome, start, and end positions in the first three columns, with subsequent columns showing the coverage for each selected reference file.
	
	Mandatory parameters:
	  -in <file>          Coverage profile of main sample in BED format.
	  -in_ref <filelist>  Reference coverage profiles of other sample in BED format (GZ files supported).
	  -out <file>         Output TSV file with coverage profiles of selected reference samples.
	
	Optional parameters:
	  -exclude <filelist> Regions in the given BED file(s) are excluded from the coverage calcualtion, e.g. copy-number polymorphic regions.
	                      Default value: ''
	  -cov_max <int>      Best n reference coverage files to include in 'out' based on correlation.
	                      Default value: '150'
	  -debug              Enable debug output.
	                      Default value: 'false'
	
	Special parameters:
	  --help              Shows this help and exits.
	  --version           Prints version and exits.
	  --changelog         Prints changeloge and exits.
	  --tdx               Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### CnvReferenceCohort changelog
	CnvReferenceCohort 2024_08-36-g4fed1f49
	
	2024-08-16 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)