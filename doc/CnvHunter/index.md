# CnvHunter documentation

CnvHunter is a CNV detection algorithm for targeted NGS data, i.e. panel or exome sequencing. It detects CNVs in one sample based on the coverage statistics determined from the other samples, i.e. the reference samples.  

##Input

Coverage profiles of sequencing data (i.e. the average coverage of each exon) for multiple samples are the input of the algorithm. 

The coverage profile for a sample can be created using the [BedCoverage](../Tools/BedCoverage.md) tool.  
It requires only the (indexed) BAM file and the target region and writes the coverage profile.
The call looks like that:

	> BedCoverage -min_mapq 0 -in target_region.bed -bam sample1.bam -out sample1.cov

The produced coverage profile is a simple tab-separated file in this format:

	#chr	start	end	coverage
	chr1	8022708	8023261	212.22
	chr1	8025241	8025765	116.72
	chr1	8029391	8029755	1.78
	...
		

## Algorithm description

CnvHunter is simply called with the coverage profiles as input:

	> CnvHunter -out output.tsv -in sample1.bam sample2.bam sample3.bam sample4.bam sample5.bam ...

These are the steps CnvHunter performs:

1.	**Normalization:** The input coverage profile of each sample is first normalized  by the mean coverage to remove differences in sequencing depth.

2.	**Region QC:** Regions that show a too low average coverage or a too high variance  in coverage are excluded from the analysis.

3.	**Reference sample construction:** For each sample, the most similar other samples are detected using the correlation of normalized coverage profiles. From the most similar  samples, a synthetic reference sample is constructed. It contains robust estimates of the average coverage and standard deviation for each exon.

4.	**Sample QC:** Samples that show a too low correlation  to the synthetic reference sample are excluded from the analysis.

5.	**CNV detection:** For each sample, regions that are coverage outliers are detected based on the z-score  calculated using the synthetic reference sample. Around these outliers, adjacent regions are included if they exceed a second (lower) z-score cutoff .

6.	**CNV refinement:** After the main CNV detection, adjacent regions that show the same copy-number are merged into one copy-number event. 

## Output

The main output of CnvHunter is a tab-separated text file that contains the copy-number events for all samples. For each event, the size in bases, the number of exons, the estimated copy-number for each region, the z-score for each region and the exon coordinates are given to allow further filtering of the output. 

	#chr	start	end	sample	size	region_count	region_copy_numbers	region_zscores	region_coordinates
	chr1	22079426	22079632	sample2	207	1	3	4.17	chr1:22079426-22079632
	chr1	25711118	25711307	sample4	190	2	1,1	-2.83,-4.34	chr1:25711118-25711208,chr1:25711217-25711307

## More information

Please also have a look at the [poster presented at ESHG 2017](CnvHunter_poster.pdf).

## Help and ChangeLog

The CnvHunter command-line help and changelog can be found [here](../tools/CnvHunter.md).

[back to ngs-bits](https://github.com/imgag/ngs-bits)

