### VcfCalculatePRS tool help
	VcfCalculatePRS (2021_06-89-gbbd16264)
	
	Calculates the Polgenic Risk Score(s) for a sample.
	
	The PRS VCF files have to contain a WEIGHT entry in the INFO column.
	Additionally some information about the PRS score is required in the VCF header.
	An example VCF file can be found at https://github.com/imgag/ngs-bits/blob/master/src/tools-TEST/data_in/VcfCalculatePRS_prs2.vcf
	
	Mandatory parameters:
	  -in <file>      Tabix indexed VCF.GZ file of a sample.
	  -prs <filelist> List of PRS VCFs.
	  -out <file>     Output TSV file containing Scores and PRS details
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfCalculatePRS changelog
	VcfCalculatePRS 2021_06-89-gbbd16264
	
	2020-07-22 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)