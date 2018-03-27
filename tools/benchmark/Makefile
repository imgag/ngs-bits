help:
	@echo "To benchmark current library/tool test times against the last three nightly tests call:"
	@echo "  > make tests - "
	@echo ""
	@echo "To benchmark a custom tool call against a previous version call:"
	@echo "  > php command.php [version_old] [tool] [arguments]"
	@echo ""
	
######################################### tests #########################################

tests:
	make test_lib test_tools benchmark
	
test_lib:
	(cd ../../ && make test_lib) > test_lib1.log
	(cd ../../ && make test_lib) > test_lib2.log
	(cd ../../ && make test_lib) > test_lib3.log

test_tools:
	(cd ../../ && make test_tools) > test_tools1.log
	(cd ../../ && make test_tools) > test_tools2.log
	(cd ../../ && make test_tools) > test_tools3.log

benchmark:
	php benchmark.php *.log > benchmark.tsv
	
clean:
	rm *.log benchmark.tsv


######################################### htslib refactoring #########################################

commands_htslib:
	php command.php 2018_03-10-gb86a37c SampleCorrelation -in /mnt/projects/diagnostic/Exome_Diagnostik/Sample_DX180848_01/DX180848_01.bam /mnt/projects/diagnostic/Exome_Diagnostik/Sample_DX180849_01/DX180849_01.bam -mode bam -max_snps 20000
	php command.php 2018_03-10-gb86a37c BamCleanHaloplex -in /mnt/projects/diagnostic/HBOC/Sample_DX151488_01/DX151488_01.bam -out /tmp/test.bam
	php command.php 2018_03-10-gb86a37c BamClipOverlap -in /mnt/projects/diagnostic/HBOC/Sample_DX151488_01/DX151488_01.bam -out /tmp/test.bam
	php command.php 2018_03-10-gb86a37c BedAnnotateFreq -in test.bed -bam /mnt/projects/diagnostic/Exome_Diagnostik/Sample_DX180848_01/DX180848_01.bam
	php command.php 2018_03-10-gb86a37c BedReadCount -in /mnt/share/data/enrichment/tsKingsmore_2013_02_19.bed -bam /mnt/projects/diagnostic/HBOC/Sample_DX151488_01/DX151488_01.bam
	php command.php 2018_03-10-gb86a37c BamToFastq -in /mnt/projects/diagnostic/HBOC/Sample_DX151488_01/DX151488_01.bam -out1 /tmp/test1.fastq.gz -out2 /tmp/test2.fastq.gz
	php command.php 2018_03-10-gb86a37c BedCoverage -in /mnt/share/data/enrichment/tsKingsmore_2013_02_19.bed -bam /mnt/projects/diagnostic/HBOC/Sample_DX151488_01/DX151488_01.bam -mode panel
	php command.php 2018_03-10-gb86a37c BamDownsample -in /mnt/projects/diagnostic/Exome_Diagnostik/Sample_DX180848_01/DX180848_01.bam -percentage 50 -out /tmp/test.bam
	php command.php 2018_03-10-gb86a37c MappingQC -in /mnt/projects/diagnostic/Exome_Diagnostik/Sample_DX180848_01/DX180848_01.bam -txt -roi /mnt/share/data/enrichment/ssHAEv6_2017_01_05.bed
	php command.php 2018_03-10-gb86a37c SomaticQC -tumor_bam /mnt/projects/diagnostic/SomaticAndTreatment/Sample_DX180139_01/DX180139_01.bam -normal_bam /mnt/projects/diagnostic/SomaticAndTreatment/Sample_DX174575_01/DX174575_01.bam  -target_bed /mnt/share/data/enrichment/ssSC_v3_2017_10_05.bed -somatic_vcf /mnt/projects/diagnostic/SomaticAndTreatment/Somatic_DX180139_01-DX174575_01/DX180139_01-DX174575_01_var_annotated.vcf -out /tmp/test.qcML
	php command.php 2018_03-10-gb86a37c VariantAnnotateFrequency -in /mnt/projects/diagnostic/Exome_Diagnostik/Sample_DX180848_01/DX180848_01.GSvar -bam /mnt/projects/diagnostic/Exome_Diagnostik/Sample_DX180848_01/DX180848_01.bam -out /tmp/test.GSvar
