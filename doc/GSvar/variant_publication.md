## Variant publication

Pathogenic variant should be uploaded to one of the public variant databases.  
With this release [ClinVar](https://www.ncbi.nlm.nih.gov/clinvar/) upload is supported and the LOVD upload was removed.

### Submitting SNVs/InDels, CNVs and SVs

Database upload for SNVs/InDels as well as CNVs and SVs (except translocations) is triggered from the context menu of variants in the main window of a **single sample** variant list.
To upload a variant the following requirements have to be fulfilled:

  - (1) The variant has to have a report configuration (and also be in the NGSD)
  - (2) The variant has to have a valid classification
  - (3) The sample has to have at least one valid OMIM/Orpha disease identifier
  - (4) The variant was not manually modified in the report configuration
  - (5) The submission of translocations is currently not supported by the ClinVar API 

The following example shows how a variant can be selected and uploaded to ClinVar:

![alt](variant_publication_context_menu.png)

The submission dialog then allows easy upload using these steps:

  - (1) Modify the list of affected genes (can be empty).
  - (2) Fill in missing variant details (e.g. affected status, allele origin). 
  - (3) Select phenotypes (these are automatically imported from GenLab if possible).
  - (4) Add additional comment for phenotype (optional).
  - (5) Fill in missing details of the clinical significance (e.g. inheritance).
  - (6) Upload to ClinVar (only possible if all required fields are filled).
  - (7) Print upload results.

![alt](variant_publication_dialog.png)

### Submitting compound-heterozygous variants

The submission of compound-heterozygous variants can be either triggered by selecting two variants in the variant widget:

![alt](variant_publication_comphet.png)

Or start with a single variant and add a compound-heterozygous variant in the submission widget:

![alt](variant_publication_comphet_add.png)

This requires a previously created report configuration of the variant which should be added.

### Submitting of manually curated variants

Variants which are manually modified can not be uploaded by the standard dialog. Please use the manual ClinVar upload at `NGSD` > `Variants` > `Manual ClinVar upload`.

![alt](variant_publication_dialog_manual.png)

Here you get a slightly modified upload dialog. This dialog has some additional entries:

  - (1) Add the sample name. This will also update the disease information if it is available in the NGSD.
  - (2) Select the variant type of the variant
  - (3) Add here a compound-heterozygous variant if needed
  - (4) Here you can add the disease info (Warning: will be overwritten if sample name is changed)

### Updating a submission

To update a submission, e.g. to change a class, open the variant publication dialog:

![alt](published_variants_open.png)

Then you can edit the submission in the context menu:

![alt](published_variants_edit.png)



### Submitting through the ClinVar Submission Portal

For the submission of variants which are not supported by the ClinVar API (e.g. translocations), use the Submission Portal of ClinVar:

[ClinVar Submission Portal](https://submit.ncbi.nlm.nih.gov/clinvar/)

In order to submit variant to Clinvar you need a NCBI account which is linked to your organization/institute. Since NCBI disabled the creation of new accounts an ORCiD account is needed beforehand. If you already have an ORCiD account you can skip the next chapter.

#### Create Orcid account

To create an ORCID account go to [https://orcid.org/](https://orcid.org/) and click on `SIGN IN/REGISTER`.
![alt](create_orcid_account_1.png)

Then select `Register now` and follow the instructions.
![alt](create_orcid_account_2.png)

#### Create NCBI account

The ORCID account can be used to login into ClinVar/NCBI and allows you to create a new NCBI account.

First go to the [ClinVar Submission Portal](https://submit.ncbi.nlm.nih.gov/clinvar/), click on `Log in` in the top right corner and choose `ORCiD`.
![alt](orcid_login.png)

On the next site log in with your ORCiD credentials and allow to link ClinVar with ORCiD.
![alt](orcid_link.png)

Now you can create a NCBI account:
![alt](create_ncbi_account.png)

Finally you have to complete your NCBI profile and your account has to be added to the group of your organization/institute. This can be done by one of the administrators of the group.

#### Upload variant to ClinVar

Log in into ClinVar and visit the [ClinVar Submission Portal](https://submit.ncbi.nlm.nih.gov/clinvar/). Here select `ClinVar single submission wizard` and go through the steps on the following pages.
![alt](submission_wizard.png)
![alt](clinvar_wizard_step1.png)

Detailed help about the single steps can be found on the ClinVar website:
 - [Detailed help for the ClinVar Submission Wizard](https://www.ncbi.nlm.nih.gov/clinvar/docs/wizard/)
 - [YouTube video](https://www.youtube.com/watch?v=IP0wr7JSvU4)


## FAQ

### There is no context menu entry for database upload.

To activate the database upload, the API key for ClinVar need to be set in the `GSvar.ini` file by the administrator.


### Phenotypes are not automatically imported from GenLab.

Make sure the field 'Labornummer' is filled in for your assay in GenLab. Otherwise the sample cannot be matched to the NGS data.

--

[back to main page](index.md)
