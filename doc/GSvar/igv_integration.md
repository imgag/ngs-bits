## IGV intregration

IGV is a genome brwoser developed at the Broad institute (see also the [IGV user guide](http://software.broadinstitute.org/software/igv/UserGuide)).  
It visualizes high-throughput sequencing data and many associated data formats, e.g.:

* BAM - contains mapped reads
* VCF - contains variants
* BED - contains regions, e.g. target regions, low-coverage regions
* SEG - copy-number calling data

GSvar can communicate with an IGV to open the data and jump to specific genomic positions.  

**Note:** GSvar cannot not start IGV. An instance of IGV has to be started manually before using the IGV integration.

### Jumping to genomic locations

Jumping to a genomic location in IGV can be triggered from several views/dialogs of GSvar:

* *variant list* - clicking the variant coordinate in the variant details view (or double-clicking a line in the variant list) opens the variant location.
* *low-coverage report dialog* - double-clicking a gap line opens the gap location.
* *CNV dialog* - double-clicking a CNV line opens the location.
* *gene selector dialog* - double-clicking a gene list opens the gene locus.

### Initializing an IGV session

When using the IGV integration for the first time, an initialization dialog is shown, in which the user can select with data files should be opened in IGV:

![igv_init.png](igv_init.png)

IGV initialization can be perfomed (`Ok`), skipped once (`Skip`), or skipped for the whole session (`Skip (session)`).  
When IGV initialization was perfomed or when was skipped for the session, the initialization dialog will not be shown for any subsequent clicks to a variant/region.

In order to force initialization, select `IGV > Reset initialization status` from the main menu, or re-load the sample.


**Note:** Custom tracks can be added by your administrator through the GSvar INI file.



## FAQ

### IGV does not open
Follow these instructions, if only the black console window of IGV but not the actual application opens.

 - Open the path `C:\Users\[login]\` in the Explorer (replace `[login]` by your Windows login).
 - Delete or rename the `igv` folder (if it cannot be deleted, close all IGV windows).
 - Restart IGV.
 - Accept the genomes cannot be loaded dialog with `ok`.
 - Change the proxy settings as described below in `IGV cannot load genomes`.

### IGV cannot load genomes
IGV needs access to the Broad Institute web server to manage non-local genome files.  
If it cannot access the server, during startup the error message `cannot connect to genome server` is shown and several other errors can occur.

In that case, you have to set the proxy like shown here:
![alt text](igv_proxy.png)

Finally, you have to restart IGV.

--
[back to main page](index.md)


