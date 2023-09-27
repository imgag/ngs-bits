### VcfAnnotateFromBigWig tool help
	VcfAnnotateFromBigWig (2023_06-98-g044e3ed3)
	
	Annotates the INFO column of a VCF with data from a bigWig file.
	
	The annotation is decided according the following rules:
	Insertions are not annotated.
	SNPs are annotated according to the corresponding value in the bigWig file. If the file has no corresponding value no annotation is written.
	MNPs, complex INDELs and Deletions the annotated value is choosen according to the given mode:
	max - maximum; min - minimum; avg - average; of the values in the affected reference region.
	none - regions that affect multiple reference bases are not annotated.
	
	Mandatory parameters:
	  -in <file>        Input VCF file. If unset, reads from STDIN.
	  -bw <file>        BigWig file containen the data to be used in the annotation.
	  -name <string>    Name of the new INFO column.
	  -mode <enum>      Annotate mode: How the annotation is chosen when multiple bases are affected.
	                    Valid: 'max,min,avg,none'
	
	Optional parameters:
	  -out <file>       Output VCF or VCF or VCF.GZ file. If unset, writes to STDOUT.
	                    Default value: ''
	  -threads <int>    The number of threads used to read, process and write files.
	                    Default value: '1'
	  -block_size <int> Number of lines processed in one chunk.
	                    Default value: '5000'
	  -prefetch <int>   Maximum number of blocks that may be pre-fetched into memory.
	                    Default value: '64'
	  -debug <int>      Enables debug output at the given interval in milliseconds (disabled by default, cannot be combined with writing to STDOUT).
	                    Default value: '-1'
	
	Special parameters:
	  --help            Shows this help and exits.
	  --version         Prints version and exits.
	  --changelog       Prints changeloge and exits.
	  --tdx             Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfAnnotateFromBigWig changelog
	VcfAnnotateFromBigWig 2023_06-98-g044e3ed3
	
	2022-01-14 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)