### VcfCalculatePRS tool help
	VcfCalculatePRS (2022_12-82-g025eb99e)
	
	Calculates the Polgenic Risk Score(s) for a sample.
	
	The PRS VCF files have to contain a WEIGHT entry in the INFO column.
	Additionally some information about the PRS score is required in the VCF header.
	An example VCF file can be found at https://github.com/imgag/ngs-bits/blob/master/src/tools-TEST/data_in/VcfCalculatePRS_prs2.vcf
	
	Mandatory parameters:
	  -in <file>      Tabix indexed VCF.GZ file of a sample.
	  -prs <filelist> List of PRS VCFs.
	  -bam <file>     BAM file corresponding to the VCF.
	  -out <file>     Output TSV file containing Scores and PRS details
	
	Optional parameters:
	  -ref <file>     Reference genome FASTA file. If unset, 'reference_genome' from the 'settings.ini' file is used.
	                  Default value: ''
	
	Special parameters:
	  --help          Shows this help and exits.
	  --version       Prints version and exits.
	  --changelog     Prints changeloge and exits.
	  --tdx           Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfCalculatePRS changelog
	VcfCalculatePRS 2022_12-82-g025eb99e
	
	2022-12-15 Added BAM depth check and population AF.
	2020-07-22 Initial version of this tool.
[back to ngs-bits](https://github.com/imgag/ngs-bits)