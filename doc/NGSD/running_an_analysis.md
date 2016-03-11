
#Running a NGS data analysis

##Creating a new processing system

To run a data analysis with our analysis pipeline, you first have to create a processing system in the NGDS:

* log into the NGSD
* to to `Admin Section` > `Processing Systems`
* press the `Add` button
* create a new processing system:
	* name: Name, can contain spaces and special characters
	* short name: Short name that can be part of file names (no spaces, brackets and other special characters)
	* adapter p5: Read 1 sequencing adapter (at least the first 20 bases)
	* adapter p7: Read 2 sequencing adapter (at least the first 20 bases)
	* shotgun: Random inserts (true), or amplicon-based inserts (false).
	* target file: Region of interest (BED file path on the linux server)
	* genome: Select 'hg19'

##Running an analysis

The analysis pipeline assumes that that all data to analyze resides in a sample folder as produced by Illumina's CASAVA tool.  
If that is the case, you can simply run:

	php php/src/Pipelines/analyze.php -folder [sample-folder] -name [sample-name] -system [processing-system] -steps ma,vc,an

After that data analysis the sample folder contains BAM and VCF (gzipped) files as expected.  
Additionally a GSvar file is created (open with the Windows GSvar) and several qcML files that contain QC data (open with a browser).

##Importing variant/QC data into the NGSD

In order to import variants and QC values of a sample into the NGSD, you have to create a processed sample in the NGSD first:

TODO

