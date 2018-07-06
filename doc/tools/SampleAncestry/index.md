# SampleAncestry documentation

The ancestry estimation is based on correlating the sample variants with population-specific SNPs.  
For each population (AFR,EUR,SAS,EAS) the 1000 most informative exonic SNPs were selected for this purpose.  
A benchmark on the 1000 Genomes variant data assigned 99.9% of the samples to the correct population (2155 of 2157).

Due to different similarity between popultations, the expected scores differ depending on the ancestry of the sample of interest.  
This plot shows the score distribution on the 1000 Genomes data:

![sample ancestry score distribution](ancestry_scores.png) 



## Help and ChangeLog

The SampleAncestry command-line help and changelog can be found [here](../SampleAncestry.md).

[back to ngs-bits](https://github.com/imgag/ngs-bits)








