### HgvsToVcf tool help
	HgvsToVcf (2024_08-110-g317f43b9)
	
	Transforms a TSV file with transcript ID and HGVS.c change into a VCF file.
	
	Transforms a given TSV file with the transcript ID (e.g. ENST00000366955) in the first column and the HGVS.c change (e.g. c.8802A>G) in the second column into a VCF file.
	Any further columns of the input TSV file are added as info entries to the output VCF. The TSV column header is used as name for the info entries.
	Ensembl, CCDS and RefSeq transcript IDs can be given, the conversion is always based on the Ensembl transcripts. CCDS and RefSeq transcripts will be matched to an Ensembl transcript, if an identical one exists.
	When an input line can't be transformed into a VCF line a warning is printed to the console.
	
	Attention: This tool is experimental. Please report any errors!
	
	Mandatory parameters:
	  -out <file>                Output VCF file.
	
	Optional parameters:
	  -in <file>                 Input TSV file. If unset, reads from STDIN.
	                             Default value: ''
	  -ref <file>                Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                             Default value: ''
	  -input_info_field <string> The input transcript ID and HGVS.c change are added to the VCF output using this INFO field name.
	                             Default value: 'HGVSc'
	  -test                      Uses the test database instead of on the production database.
	                             Default value: 'false'
	  -build <enum>              Genome build
	                             Default value: 'hg38'
	                             Valid: 'hg19,hg38'
	  -max_seq <int>             If set, skips variants with ref/alt sequence longer than this cutoff.
	                             Default value: '-1'
	
	Special parameters:
	  --help                     Shows this help and exits.
	  --version                  Prints version and exits.
	  --changelog                Prints changeloge and exits.
	  --tdx                      Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]          Settings override file (no other settings files are used).
	
### HgvsToVcf changelog
	HgvsToVcf 2024_08-110-g317f43b9
	
	2022-07-25 Added parameter 'max_seq'.
	2022-05-12 Initial version
[back to ngs-bits](https://github.com/imgag/ngs-bits)