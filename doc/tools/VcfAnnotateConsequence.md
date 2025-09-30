### VcfAnnotateConsequence tool help
	VcfAnnotateConsequence (2025_07-127-g60fc6b39)
	
	Adds transcript-specific consequence predictions to a VCF file.
	
	Variants are normalized accoring to the HGVS 3' rule to assess the effect, i.e. they are moved as far as possible in translation direction.
	Note: HGVS consequence calcualtion is only done if the variant is fully inside the transcript region.
	
	Mandatory parameters:
	  -in <file>               Input VCF file to annotate.
	  -gff <file>              Ensembl-style GFF file with transcripts, e.g. from https://ftp.ensembl.org/pub/release-115/gff3/homo_sapiens/Homo_sapiens.GRCh38.115.gff3.gz.
	  -out <file>              Output VCF file annotated with predicted consequences for each variant.
	
	Optional parameters:
	  -ref <file>              Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                           Default value: ''
	  -threads <int>           The number of threads used to read, process and write files.
	                           Default value: '1'
	  -block_size <int>        Number of lines processed in one chunk.
	                           Default value: '5000'
	  -prefetch <int>          Maximum number of blocks that may be pre-fetched into memory.
	                           Default value: '64'
	  -all                     If set, all transcripts are used for annotation. The default is to skip transcripts not labeled with 'gencode_basic' and not labeled with 'RefSeq'/'BestRefSeq' origin for Refseq.
	                           Default value: 'false'
	  -skip_not_hgnc           Skip genes that do not have a HGNC identifier.
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
	  -source <enum>           GFF source.
	                           Default value: 'ensembl'
	                           Valid: 'ensembl,refseq'
	  -debug                   Enable debug output
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### VcfAnnotateConsequence changelog
	VcfAnnotateConsequence 2025_07-127-g60fc6b39
	
	2024-07-26 Added support for RefSeq GFF format (source parameter).
	2022-07-07 Change to event-driven multithreaded implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)