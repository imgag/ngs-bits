# MappingQC documentation

## Panel/WES

For panes or exome sequencing data, use this command:

 	> MappingQC -in [bam] -roi [target_region] [options]

## WGS

For whole genome sequencing data, use this command:

	> MappingQC -in [bam] -wgs [options]

## RNA

For transcriptome sequencing data, use this command:

	> MappingQC -in [bam] -rna [options]

## Runtime

Generally, the runtime on BAM is faster than on CRAM because of the the more complex decompression for CRAMs.

For small panels, the expected runtime below a minute (including `-read_qc`). 
For 100x WES, the expected runtime is about 5 minutes (including `-read_qc`, which is 2-3 minutes).  
For 30x WGS, the expected runtime is about 30 minutes (including `-read_qc`, which is about 20 minutes).  


## Help and ChangeLog

The full list of command-line parameters and changelog can be found [here](../MappingQC.md).

[back to ngs-bits](https://github.com/imgag/ngs-bits)
