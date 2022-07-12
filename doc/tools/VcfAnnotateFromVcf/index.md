# VcfAnnotateFromVcf documentation

## Single annotation source file

When annotating from a single source VCF file, you can do that using command line parameters only.

If the annotation source file *source.vcf.gz* contains the INFO keys A, B and C. You can annotate them to the input using:

        > VcfAnnotateFromVcf -in [input].vcf[.gz] -source source.vcf.gz -threads [threads] -info_ids A,B,C -out [output].vcf

If you want to annotate A, but want to rename the key to D, use this command:

        > VcfAnnotateFromVcf -in [input].vcf[.gz] -source source.vcf.gz -threads [threads] -info_ids A=D -out [output].vcf

If you want to annotate A, B and C and the ID column, use this command:

        > VcfAnnotateFromVcf -in [input].vcf[.gz] -source source.vcf.gz -threads [threads] -info_ids A,B,C -id_column ID -out [output].vcf


## Multiple annotation source files

If you want to annotate information from several source VCF files, you can provide a config file:

        > VcfAnnotateFromVcf -in [input].vcf[.gz] -config_file [config_file] -threads [threads] -out [output].vcf


The config file is a tab-separated file that contains one line for each annoatation source file.  
The following columns are required in each line:

	- source VCF file (see parameter 'source')
	- output name prefix (see parameter 'prefix', optional)
	- INFO key names (see parameter 'info_keys', optional)
	- ID column name (see parameter 'id_column', optional)

## Help and ChangeLog

The full parameter list and changelog can be found [here](../VcfAnnotateFromVcf.md).

[back to ngs-bits](https://github.com/imgag/ngs-bits)
