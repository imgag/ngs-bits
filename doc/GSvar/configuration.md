# GSvar configuration

## INI files

The main configuration of `GSvar` is done via INI files located in the same folder as the `GSvar` executable.

### settings.ini

This file contains basic settings that are used for all `ngs-bits` applications. If it is missing, copy the example file:

`cp settings.ini.example settings.ini`

The settings are:

- *reference_genome*: the path to the reference genome FASTA file
- *ngsd\_...*: Database credentials for the NGSD (if available)
- *genlab\_...*: Database credentials for the GenLab (if available)
- *projects\_folder*: Main projects folder where sample data is stored
- *target\_file\_folder_...*: Processing system target BED file folder

### GSvar.ini

This file contains settings specific for `GSvar`. If it is missing, copy the example file:

`cp GSvar.ini.example GSvar.ini`

The most important settings are:

- *lovd\_...*: LOVD credentials for upload of variants
- *analysis\_steps\_...*: Analysis steps of queuing of analysis on cluster
- *proxy\_...*: Proxy settings (if needed)
- *gsvar\_...*: Output folders for different documents `GSvar` creates, e.g. reports.
- *igv\_menu*: A comma separated list of IGV custom tracks. Each track consists of these tab-separated parts:
	- track name
	- checked by default (0 or 1)
	- path to track file
- *email\_run\_...*: semicolon-separated list of additional email adresses that are used when the email button on the run tab is pressed.

## Filters

Default filters can be defined using these files, located in the same folder as the `GSvar` executable.

### GSvar\_filters.ini

Default filters for small variants.  

Lines that start with `#` define a filter name. All lines after that don't start with `#` contain filter steps.  

Lines that contain  `#---` insert a separator. They are used to group filters.

### GSvar\_filters\_cnv.ini

Default filters for CNVs.  

Lines that start with `#` define a filter name. All lines after that don't start with `#` contain filter steps.  

Lines that contain  `#---` insert a separator. They are used to group filters.

## Other files

### GSvar\_preferred\_transcripts.tsv

This file contains [preferred transcripts](preferred_transcripts.md).

Each line starts the a gene name followed by one or more ENST transcripts without version number.

*Note:* The separator used is tab, also between transcripts!

### GSvar\_special\_regions.tsv

This file contains special regions for sub-panel design.

Normally, sub-panels are designed a coding regions plus 5 to 20 flanking bases. However, some genes contain known pathogenic variants in intronic or intergenic regions. These non-coding but important regions can be specified in this file.

Each line starts the a gene name followed by one or more regions in the format `chr:start-end`.

*Note:* The separator used is tab, also between regions!
