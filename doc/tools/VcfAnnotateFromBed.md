### VcfAnnotateFromBed tool help
	VcfAnnotateFromBed (2021_03-100-g09f25a25)
	
	Annotates the INFO column of a VCF with data from a BED file.
	
	Characters which are not allowed in the INFO column based on the VCF 4.2 definition are URL encoded.
	The following characters are replaced:
	% -> %25; 	 -> %09;
	 -> %0A;  -> %0D;   -> %20; , -> %2C; ; -> %3B; = -> %3D;
	
	Mandatory parameters:
	  -bed <file>    BED file used as source of annotations (name column).
	  -name <string> Annotation name in INFO column of output VCF file.
	
	Optional parameters:
	  -in <file>     Input VCF file. If unset, reads from STDIN.
	                 Default value: ''
	  -out <file>    Output VCF list. If unset, writes to STDOUT.
	                 Default value: ''
	  -sep <string>  Separator used if there are several matches for one variant.
	                 Default value: ':'
	
	Special parameters:
	  --help         Shows this help and exits.
	  --version      Prints version and exits.
	  --changelog    Prints changeloge and exits.
	  --tdx          Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### VcfAnnotateFromBed changelog
	VcfAnnotateFromBed 2021_03-100-g09f25a25
	
	2021-06-15 Added 'sep' parameter.
	2019-12-06 Added URL encoding for INFO values.
	2017-03-14 Initial implementation.
[back to ngs-bits](https://github.com/imgag/ngs-bits)