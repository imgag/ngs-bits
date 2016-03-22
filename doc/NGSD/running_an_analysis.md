
#Running a NGS data analysis

##Downloading test data

For the first data anlysis you first some test data.  
You can download an example dataset of the NIST reference sample NA12878 [here](https://medgen.medizin.uni-tuebingen.de/NGS-downloads/NA12878_01.zip).


##Creating a new processing system

To run a data analysis with our analysis pipeline, you first have to create a processing system in the NGDS:

* log into the NGSD
* to to `Admin Section` > `Processing Systems`
* press the `Add` button
* create a new processing system (see `processing_system.txt` file in the downloaded test data):
	* name: Name, can contain spaces and special characters
	* short name: Short name that can be part of file names (no spaces, brackets and other special characters)
	* adapter p5: Read 1 sequencing adapter (at least the first 20 bases)
	* adapter p7: Read 2 sequencing adapter (at least the first 20 bases)
	* shotgun: Random inserts (true), or amplicon-based inserts (false).
	* target file: Region of interest (BED file path on the linux server)
	* genome: Select 'hg19'

##Running an analysis

The analysis pipeline assumes that that all data to analyze resides in a sample folder as produced by Illumina's CASAVA tool. If that is the case, the whole analysis is performed with one command.  
For example, the command to analyze the NA12878 test data is this:

	php php/src/Pipelines/analyze.php -folder Sample_NA12878_01 -name NA12878_01 -system hpHBOCv5 -steps ma,vc,an

After the data analysis, the sample folder contains BAM and VCF (gzipped) files as expected.  
Additionally a GSvar file is created (open with the GSvar tool) and several qcML files that contain QC data (open with a browser).

##Importing variant/QC data into the NGSD

In order to import variants and QC values of a sample into the NGSD, you have to create a processed sample in the NGSD first:

 * First, go to the `Admin section` and create:
  * A `device` (sequencer).
  * A `sender` for the sample.
 * Create a `project`:
  * Use 'admin' as internal coordinator.
 * Create a `sequencing run`:
  * The 'recipe' is the read length(s), e.g. '100+8+100'
  * Use the 'device' you created above.
 * Create a `sample`:
  * Name it 'NA12878'.
  * Use the 'sender' you created above.
  * Select 'DNA' as sample type.
  * Select 'human' as species.
 * Create a `processed sample`:
  * Use 'NA12878' as sample.
  * Use the 'project' you created above.
  * Use the 'run' you created above.
  * Use the 'processing system' you created above.

Now, you can import the variant and QC data of the sample `NA12878_01`into the NGSD using the following command:

	php php/src/Pipelines/analyze.php -folder Sample_NA12878_01 -name NA12878_01 -system hpHBOCv5 -steps db






