### VcfAnnotateFrequency tool help
	VcfAnnotateFrequency (2025_12-49-gdb0c5d35)
	
	Annotates VCF variants with allele frequency from a BAM/CRAM file.
	
	Mandatory parameters:
	  -in <file>        Input variant list to annotate in VCF(.GZ) format.
	  -bam <file>       Input BAM/CRAM file.
	  -out <file>       Output variant list file in VCF format.
	
	Optional parameters:
	  -depth            Annotate an additional INFO field entry containing the depth.
	                    Default value: 'false'
	  -name <string>    INFO field entry prefix in output file.
	                    Default value: 'N'
	  -ref <file>       Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                    Default value: ''
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### VcfAnnotateFrequency changelog
	VcfAnnotateFrequency 2025_12-49-gdb0c5d35
	
	2025-11-28 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)