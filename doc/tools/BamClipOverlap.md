### BamClipOverlap tool help
	BamClipOverlap (2024_08-110-g317f43b9)
	
	Softclipping of overlapping reads.
	
	Overlapping reads will be soft-clipped from start to end. There are several parameters available for handling of mismatches in overlapping reads. Within the overlap the higher base quality will be kept for each basepair.
	
	Mandatory parameters:
	  -in <file>                Input BAM/CRAM file. Needs to be sorted by name.
	  -out <file>               Output BAM file.
	
	Optional parameters:
	  -overlap_mismatch_mapq    Set mapping quality of pair to 0 if mismatch is found in overlapping reads.
	                            Default value: 'false'
	  -overlap_mismatch_remove  Remove pair if mismatch is found in overlapping reads.
	                            Default value: 'false'
	  -overlap_mismatch_baseq   Reduce base quality if mismatch is found in overlapping reads.
	                            Default value: 'false'
	  -overlap_mismatch_basen   Set base to N if mismatch is found in overlapping reads.
	                            Default value: 'false'
	  -ignore_indels            Turn off indel detection in overlap.
	                            Default value: 'false'
	  -v                        Verbose mode.
	                            Default value: 'false'
	  -ref <file>               Reference genome for CRAM support (mandatory if CRAM is used).
	                            Default value: ''
	
	Special parameters:
	  --help                    Shows this help and exits.
	  --version                 Prints version and exits.
	  --changelog               Prints changeloge and exits.
	  --tdx                     Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]         Settings override file (no other settings files are used).
	
### BamClipOverlap changelog
	BamClipOverlap 2024_08-110-g317f43b9
	
	2020-11-27 Added CRAM support.
	2018-01-11 Updated base quality handling within overlap.
	2017-01-16 Added overlap mismatch filter.
[back to ngs-bits](https://github.com/imgag/ngs-bits)