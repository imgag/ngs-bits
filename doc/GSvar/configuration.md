# GSvar configuration

## INI files

The main configuration of `GSvar` is done via INI files located in the same folder as the `GSvar` executable.

Since these files can contain sensitive information like passwords, it is possible to [encrypt information in the settings file](encrypt_settings.md).

GSvar is usually running in client-server mode. Thus, you need to [configure the server](../GSvarServer/index.md) as well.

### settings.ini

This file contains basic settings that are used for all `ngs-bits` applications. If it is missing, copy the example file:

`cp settings.ini.example settings.ini`

The settings are:

- *reference_genome*: the path to the reference genome FASTA file
- *ngsd_...*: Database credentials for the NGSD (if available)
- *db_ssl_ca: SSL certificate authorities needed to validate connections to the database server
- *genlab_...*: Database credentials for the GenLab (if available)
- *projects_folder_...*: Prject data folders for different project types (diagnostic, research, test, external)
- *data_folder*: megSAP data folder used to find target region BED files of processing systems

### GSvar.ini

This file contains settings specific for `GSvar`. If it is missing, copy the example file:

`cp GSvar.ini.example GSvar.ini`

The most important settings are:

- *build*: Genome build to use. Default is `hg38`.
- *threads*: number of threads used
- *clinvar_api_key.*: API key for Clinvar upload of variants
- *analysis_steps_...*: Analysis steps of queuing of analysis on cluster for different analysis types
- *proxy_...*: Proxy settings (if GSvar is running behind a proxy server)
- *gsvar_...*: Output folders for different documents `GSvar` creates, e.g. reports.
- *email_run_...*: semicolon-separated list of additional email addresses that are used when the email button on the run tab is pressed.
- *custom_menu_small_variants*: tab-separated list of custom context menu entries. Each entry constists of `name|URL`. The name is shown in the context menu. The URL is opened and the following strings are replaced by values from the variant: `[chr]`, `[start]`, `[end]`, `[ref]`, `[obs]`.
- *gsvar_file_outdated_before*: GSvar files created before the given date (yyyy-mm-dd) cause a user warning stating that they are outdated.
- *interpretability_regions*: A comma separated list of low interpretability regions. Each region consists of these tab-separated parts:
	- region name
	- path to BED file
- *text_editor*: Path of the preferred text editor. It is e.g. used to open log files.
- *use_free_hgmd_version*: If HGMD links are for free version or for licensed version (login required).
- *HerediVar*: URL of the HerediVar webservice.
To allow IGV integration, you have to provide some IGV settings:

- *igv_app*: Path to `igv.bat`.
- *igv_menu*: A comma separated list of IGV custom tracks. Each track consists of these tab-separated parts:
	- track name
	- checked by default (0 or 1)
	- path to track file
- *igv_host*: IGV host name.
- *igv_port*: IGV port.
- *igv_genome*: Path to `hg38_ensembl_masked.json`. For details check the [GSvar installation instructions](https://github.com/imgag/ngs-bits/blob/master/doc/install_win.md#building-a-custom-genome-for-igv).
- *igv_virus_genome*: Path to `somatic_viral.fa`. Download from the [megSAP repository](https://github.com/imgag/megSAP/blob/master/data/genomes/somatic_viral.fa).

If you want to run GSvar with a GSvar server, you need to provide these settings as well:

- *server_host*: Server name.
- *server_port*: Server port.
- *curl_ca_bundle*: Path to the certificate bundle `.crt` file used by the server. If not provided, the GSvar and/or IGV cannot access BAM files over HTTPS.
- *display_user_notifications*: Enable/Disable showing user notifications from the server in the client.
- *use_proxy_for_gsvar_server*: for using an external GSvar server should be set to True (if your network is behind a HTTP proxy server).


## Filters

Default filters can be defined using these files, located in the same folder as the `GSvar` executable.

### GSvar_filters.ini

Default filters for small variants.  

Lines that start with `#` define a filter name. All lines after that don't start with `#` contain filter steps.  

Lines that contain  `#---` insert a separator. They are used to group filters.

Filter names and parameters are described [here](../tools/VariantFilterAnnotations.md).

### GSvar_filters_cnv.ini

Default filters for CNVs.  

Lines that start with `#` define a filter name. All lines after that don't start with `#` contain filter steps.  

Lines that contain  `#---` insert a separator. They are used to group filters.

Filter names and parameters are described [here](../tools/CnvFilterAnnotations.md).

### GSvar_filters_sv.ini

Default filters for SVs.  

Lines that start with `#` define a filter name. All lines after that don't start with `#` contain filter steps.  

Lines that contain  `#---` insert a separator. They are used to group filters.

Filter names and parameters are described [here](../tools/SvFilterAnnotations.md).

## Other files

### GSvar_special_regions.tsv

This file contains special regions for sub-panel design.

Normally, sub-panels are designed a coding regions plus 5 to 20 flanking bases. However, some genes contain known pathogenic variants in intronic or intergenic regions. These non-coding but important regions can be specified in this file.

Each line starts the a gene name followed by one or more regions in the format `chr:start-end`.

*Note:* The separator used is tab, also between regions!

--

[back to main page](index.md)
