## Variant publication

Novel pathogenic variant should be uploaded to one of the public variant databases.  
Currenlty only [LOVD](http://www.lovd.nl/3.0/home) is supported - [ClinVar](https://www.ncbi.nlm.nih.gov/clinvar/) support is planned.
 
Database upload is triggered from the context menu of variants in the main window:

![alt](variant_publication_context_menu.png)

The submission dialog then allows easy upload using these steps:

- (1) Fill in missing variant details (as created by the external webservice).
- (2) Select phenotypes (these are automatically imported from GenLab if possible).
- (3) Data upload.
- (4) Printing the upload result 

![alt](variant_publication_dialog.png)


## FAQ

### There is no context menu entry for database upload.

To activate the database upload, the credentials for LOVD need to be set in the `GSvar.ini` file by the administrator.


### Phenotypes are not automatically imported from GenLab.

Make sure the the field 'Labornummer' is filled in for your assay in GenLab. Otherwise the sample cannot be matched to the NGS data.


--

[back to main page](index.md)








