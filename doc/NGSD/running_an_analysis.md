
#Running a NGS data analysis

##Downloading test data

For the first data anlysis you first some test data.  
You can download an example dataset of the NIST reference sample NA12878 [here](https://medgen.medizin.uni-tuebingen.de/NGS-downloads/NA12878_01.zip).

##Running an analysis

The analysis pipeline assumes that that all data to analyze resides in a sample folder as produced by Illumina's [bcl2fastq](http://support.illumina.com/sequencing/sequencing_software/bcl2fastq-conversion-software.html) tool. If that is the case, the whole analysis is performed with one command.  
For example, the command to analyze the NA12878 test data is this:

	php php/src/Pipelines/analyze.php -folder Sample_NA12878_01 -name NA12878_01 -system hpHBOCv5.ini -steps ma,vc,an

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
 * Create a `processing system`:
  * For this stop, use the information from the `hpHBOC.ini` file download above.
 * Create a `processed sample`:
  * Use 'NA12878' as sample.
  * Use the 'project' you created above.
  * Use the 'run' you created above.
  * Use the 'processing system' you created above.

Now, you can import the variant and QC data of the sample `NA12878_01`into the NGSD using the following command:

	php php/src/Pipelines/analyze.php -folder Sample_NA12878_01 -name NA12878_01 -system hpHBOCv5.ini -steps db







