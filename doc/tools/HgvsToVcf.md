### HgvsToVcf tool help
	HgvsToVcf (2022_04-120-g0b2ddab9)
	
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
	
	Special parameters:
	  --help                     Shows this help and exits.
	  --version                  Prints version and exits.
	  --changelog                Prints changeloge and exits.
	  --tdx                      Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### HgvsToVcf changelog
	HgvsToVcf 2022_04-120-g0b2ddab9
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)