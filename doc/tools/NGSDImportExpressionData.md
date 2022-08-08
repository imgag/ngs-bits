### NGSDImportExpressionData tool help
	NGSDImportExpressionData (2022_07-37-g22d4e20c)
	
	Imports expression data into the NGSD.
	
	Mandatory parameters:
	  -expression <file> TSV file containing expression values (TPM).
	  -ps <string>       Processed sample name of the expression data.
	
	Optional parameters:
	  -mode <enum>       Determines which kind of expression data should be imported.
	                     Default value: 'genes'
	                     Valid: 'genes,exons'
	  -force             Import data even if already imported and overwrite data in the NGSD.
	                     Default value: 'false'
	  -test              Uses the test database instead of on the production database.
	                     Default value: 'false'
	  -debug             Enable debug output.
	                     Default value: 'false'
	
	Special parameters:
	  --help             Shows this help and exits.
	  --version          Prints version and exits.
	  --changelog        Prints changeloge and exits.
	  --tdx              Writes a Tool Definition Xml file. The file name is the application name with the suffix '.tdx'.
	
### NGSDImportExpressionData changelog
	NGSDImportExpressionData 2022_07-37-g22d4e20c
	
	2022-07-18 Added exon support and removed transcripts.
	2022-06-17 Added transcript support.
	2022-05-03 Initial version.
[back to ngs-bits](https://github.com/imgag/ngs-bits)