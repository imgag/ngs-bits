### VcfAnnotateConsequence tool help
	VcfAnnotateConsequence (2022_04-120-g0b2ddab9)
	
	Adds transcript-specific consequence predictions to a VCF file.
	
	Mandatory parameters:
	  -in <file>               Input VCF file to annotate.
	  -gff <file>              Ensembl-style GFF file with transcripts, e.g. from ftp://ftp.ensembl.org/pub/release-104/gff3/homo_sapiens/Homo_sapiens.GRCh38.104.chr.gff3.gz.
	  -out <file>              Output VCF file annotated with predicted consequences for each variant.
	
	Optional parameters:
	  -ref <file>              Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                           Default value: ''
	  -all                     If set, all transcripts are imported (the default is to skip transcripts not labeled with the 'GENCODE basic' tag).
	                           Default value: 'false'
	  -tag <string>            Tag that is used for the consequence annotation.
	                           Default value: 'CSQ'
	  -max_dist_to_trans <int> Maximum distance between variant and transcript.
	                           Default value: '5000'
	  -splice_region_ex <int>  Number of bases at exon boundaries that are considered to be part of the splice region.
	                           Default value: '3'
	  -splice_region_in5 <int> Number of bases at intron boundaries (5') that are considered to be part of the splice region.
	                           Default value: '20'
	  -splice_region_in3 <int> Number of bases at intron boundaries (3') that are considered to be part of the splice region.
	                           Default value: '20'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfAnnotateConsequence changelog
	VcfAnnotateConsequence 2022_04-120-g0b2ddab9
	
[back to ngs-bits](https://github.com/imgag/ngs-bits)