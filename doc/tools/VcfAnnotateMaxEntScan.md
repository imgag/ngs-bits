### VcfAnnotateMaxEntScan tool help
	VcfAnnotateMaxEntScan (2025_07-127-g60fc6b39)
	
	Annotates a VCF file with MaxEntScan scores.
	
	This is essentially a multithreaded C++ reimplementation of the MaxEntScan plugin for VEP (https://github.com/Ensembl/VEP_plugins/blob/release/109/MaxEntScan.pm). MaxEntScan was first introduced by Shamsani et al. (https://doi.org/10.1093/bioinformatics/bty960).
	Benchmarking of this tool showed that it is up to 10x faster than the VEP plugin when using one thread.
	The standard MES scores are only computed for variants which are close to known splice sites (which are computed from the provided GFF file).
	De-novo splice sites can be found by using the MES scores from the sliding window approach (SWA).
	Intergenic variants never get a MES scores.
	
	Mandatory parameters:
	  -gff <file>        Ensembl-style GFF file with transcripts, e.g. from https://ftp.ensembl.org/pub/release-115/gff3/homo_sapiens/Homo_sapiens.GRCh38.115.gff3.gz.
	
	Optional parameters:
	  -out <file>        Output VCF file containing the MaxEntScan scores in the INFO column. If unset, writes to STDOUT.
	                     Default value: ''
	  -in <file>         Input VCF file. If unset, reads from STDIN.
	                     Default value: ''
	  -swa               Enables sliding window approach, i.e. predictions of de-novo acceptor/donor sites.
	                     Default value: 'false'
	  -all               If set, all transcripts are used for annotation (the default is to skip transcripts not labeled with the 'GENCODE basic' tag).
	                     Default value: 'false'
	  -tag <string>      Info entry name used for native splice site scores.
	                     Default value: 'MES'
	  -tag_swa <string>  Info entry name used for SWA scores.
	                     Default value: 'MES_SWA'
	  -decimals <int>    Number of decimals of output scores.
	                     Default value: '2'
	  -min_score <float> Minimum score to report.
	                     Default value: '-1000'
	  -threads <int>     The number of threads used to process VCF line chunk.
	                     Default value: '1'
	  -block_size <int>  Number of VCF lines processed in one chunk.
	                     Default value: '10000'
	  -prefetch <int>    Maximum number of chunks that may be pre-fetched into memory.
	                     Default value: '64'
	  -ref <file>        Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.
	                     Default value: ''
	  -debug             Enables debug output (use only with one thread).
	                     Default value: 'false'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]  Settings override file (no other settings files are used).
	
### VcfAnnotateMaxEntScan changelog
	VcfAnnotateMaxEntScan 2025_07-127-g60fc6b39
	
	2025-03-20 Fixed bug in SWA alternative sequence generation and using 'min_score' for comp scores as well.
	2023-09-26 Added several parameters to make output more configurable.
	2023-09-18 first version
[back to ngs-bits](https://github.com/imgag/ngs-bits)