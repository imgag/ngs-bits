### SampleIdentity tool help
	SampleIdentity (2025_12-104-g15d04bf0)
	
	Tries to identify datasets that are from the same patient based on BAM/CRAM files of WGS/WES/lrGS/RNA sequencing.
	
	This tool works for HG38 only!
	It calculates the identity of 75 SNPs that are usually well covered in lrGS, WGS, WES and RNA. It is much faster and memory-efficient than the SampleSimilarity tool, but does not work for panels. It should be used for checking for sample identitiy only. It cannot give information about relatedness of samples.
	
	Mandatory parameters:
	  -bams <filelist>         Input BAM/CRAM files. If only one file is given, it must be a text file with one BAM/CRAM path per line.
	
	Optional parameters:
	  -out <file>              Output TSV file. If unset, writes to STDOUT.
	                           Default value: ''
	  -min_depth <int>         Minimum depth to use a SNP for the sample comparison.
	                           Default value: '15'
	  -min_snps <int>          Minimum SNPs required to comare samples.
	                           Default value: '40'
	  -min_identity <int>      Minimum identity percentage to show sample pairs in output.
	                           Default value: '95'
	  -min_correlation <float> Minimum correlation to show sample pairs in output.
	                           Default value: '0.9'
	  -ref <file>              Reference genome for CRAM support (mandatory if CRAM is used).
	                           Default value: ''
	  -basename                Use BAM/CRAM basename instead of full path in output.
	                           Default value: 'false'
	  -debug                   Add debug output to STDOUT. If used, make sure to provide a file for 'out'!
	                           Default value: 'false'
	  -time                    Add timing output to STDOUT. If used, make sure to provide a file for 'out'!
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### SampleIdentity changelog
	SampleIdentity 2025_12-104-g15d04bf0
	
	2026-02-09 Initial commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)