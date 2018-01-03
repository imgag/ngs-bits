## Variant publication

Novel pathogenic variant should be uploaded to one of the public variant databases.  
Currenlty only [LOVD](http://www.lovd.nl/3.0/home) is supported - [ClinVar](https://www.ncbi.nlm.nih.gov/clinvar/) support is planned.
 
Database upload is triggered from the context menu of variants in the main window.  
The following example shows how a two heterozygous variants can be selected and uploaded as compound-heterozygous:

![alt](variant_publication_context_menu.png)

The submission dialog then allows easy upload using these steps:

- (1) Fill in missing variant details (as created by the external webservice).
- (2) Fill in missing variant details of the second variant (only for compound-heterozygous variants).
- (3) Select phenotypes (these are automatically imported from GenLab if possible).
- (4) Data upload.
- (5) Printing the upload result 

![alt](variant_publication_dialog.png)


## FAQ

### There is no context menu entry for database upload.

To activate the database upload, the credentials for LOVD need to be set in the `GSvar.ini` file by the administrator.


### Phenotypes are not automatically imported from GenLab.

Make sure the the field 'Labornummer' is filled in for your assay in GenLab. Otherwise the sample cannot be matched to the NGS data.

### Gender, genotype or classification information is missing.

Make sure that all information is present in the NGSD *before* opening the LOVD upload dialog.  
Classification data is taked from the variant list. Thus, make sure it is up-to-date using the re-annotation button in the main tool bar of GSvar.


--

[back to main page](index.md)










