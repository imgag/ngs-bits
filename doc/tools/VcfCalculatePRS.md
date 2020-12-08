### VcfCalculatePRS tool help
	VcfCalculatePRS (2020_06-40-g7bb4fdf2)
	
	Calculates the Polgenic Risk Score for a given set of PRS VCFs
	
	Mandatory parameters:
	  -in <file>      Tabix indexed VCF.GZ file of the sample.
	  -prs <filelist> List of PRS VCFs.
	  -out <file>     Output TSV file containing Scores and PRS details
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfCalculatePRS changelog
	VcfCalculatePRS 2020_06-40-g7bb4fdf2
	
	2020-07-22 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)