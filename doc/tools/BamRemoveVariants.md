### BamRemoveVariants tool help
	BamRemoveVariants (2025_01-25-g1d2b52ea)
	
	Removes reads which contain the provided variants
	
	Mandatory parameters:
	  -in <file>        Input BAM/CRAM file.
	  -out <file>       Output BAM/CRAM file.
	  -vcf <file>       Input indexed VCF.GZ file.
	
	Optional parameters:
	  -ref <file>       Reference genome for CRAM support (mandatory if CRAM is used).
	                    Default value: ''
	  -mask             Replace variant bases with reference instead of removing the read (SNV only)
	                    Default value: 'false'
	  -single_end       Input file is from single-end sequencing (e.g. lrGS).
	                    Default value: 'false'
	  -keep_indels      Do not remove InDels in mask mode.
	                    Default value: 'false'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file] Settings override file (no other settings files are used).
	
### BamRemoveVariants changelog
	BamRemoveVariants 2025_01-25-g1d2b52ea
	
	2025-01-20 Added single-end mode.
	2025-01-17 Added mask option.
	2024-07-24 Inital commit.
[back to ngs-bits](https://github.com/imgag/ngs-bits)