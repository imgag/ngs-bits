### UpdHunter tool help
	UpdHunter (2024_08-110-g317f43b9)
	
	UPD detection from trio variant data.
	
	Mandatory parameters:
	  -in <file>               Input VCF file of trio.
	  -c <string>              Header name of child.
	  -f <string>              Header name of father.
	  -m <string>              Header name of mother.
	  -out <file>              Output TSV file containing the detected UPDs.
	
	Optional parameters:
	  -out_informative <file>  Output IGV file containing informative variants.
	                           Default value: ''
	  -exclude <file>          BED file with regions to exclude, e.g. copy-number variant regions.
	                           Default value: ''
	  -var_min_dp <int>        Minimum depth (DP) of a variant (in all three samples).
	                           Default value: '20'
	  -var_min_q <float>       Minimum quality (QUAL) of a variant.
	                           Default value: '30'
	  -var_use_indels          Also use InDels. The default is to use SNVs only.
	                           Default value: 'false'
	  -ext_marker_perc <float> Percentage of markers that can be spanned when merging adjacent regions .
	                           Default value: '1'
	  -ext_size_perc <float>   Percentage of base size that can be spanned when merging adjacent regions.
	                           Default value: '20'
	  -reg_min_kb <float>      Mimimum size in kilo-bases required for a UPD region.
	                           Default value: '1000'
	  -reg_min_markers <int>   Mimimum number of UPD markers required in a region.
	                           Default value: '15'
	  -reg_min_q <float>       Mimimum Q-score required for a UPD region.
	                           Default value: '20'
	  -debug                   Enable verbose debug output.
	                           Default value: 'false'
	
	Special parameters:
	  --help                   Shows this help and exits.
	  --version                Prints version and exits.
	  --changelog              Prints changeloge and exits.
	  --tdx                    Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	  --settings [file]        Settings override file (no other settings files are used).
	
### UpdHunter changelog
	UpdHunter 2024_08-110-g317f43b9
	
	2024-06-06 Added optional output file containing informative variants.
	2020-08-07 VCF files only as input format for variant list.
	2018-06-11 First working version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)