# Running GSvar and GSvarServer inside a cloud service

`GSvar` is a desktop client application (which can be built for Windows, Mac, and Linux). It communicates with the `GSvarServer` over HTTPS.  
The `GSvarServer` application can be deployed (among other places) inside the Amazon Cloud or practically any other cloud provider  (e.g. Azure, Google Cloud, Hetzner, Digital Ocean, etc.).

## AWS deployment steps

1. Create an EC2 Linux instance. We recommed to use Ubuntu or Debian. CPU and RAM parameters may vary significantly, depending on your load. The server can be started even on a minimal instance. However, the compilation requires around 16GB of RAM. Follow the instructions from the [main page](index.md) to build the server.

2. Provide a valid SSL certificate and key for your domain name. Self-signed certificate may be used, but all the functionality based on `htslib` will not be avaliable, since `htslib` will try to validate your certificate and fail.

3. Sample data should be stored in a S3 bucket. Use `s3fs` utility to mount a bucket as a regular folder. Install and configure it first 
    ```
    sudo apt install s3fs
    echo ACCESS_KEY_ID:SECRET_ACCESS_KEY > ${HOME}/.passwd-s3fs
    chmod 600 ${HOME}/.passwd-s3fs
    ```

4. Mount S3 bucket as a folder
    ```
    s3fs S3_BUCKET_NAME FOLDER_WHERE_MOUNTED_TO -o allow_other -o passwd_file=.passwd-s3fs
    ```

5. Create a `systemd` to run `GSvarServer`, instead of manually starting `./GSvarServer`. Follow the instructions from [here](systemd.md) to achieve it.

6. The server deployment is finished

## Deployment on Hetzner

The procedure is identical to the deployement on a Linux server in AWS, except for the S3 bucket mounting (if you are using a regular folder on your server).

## Installing GSvar on a client machine

To provide a fully functional environment, a client machine should have the following items:

* GSvar binary file (on Windows it will be `GSvar.exe`)
* GSvar dependency libraries (on Windows they are located in a ./bin directoory together with `GSvar.exe`)
* IGV with Java (can be downloaded from the [official web page](https://igv.org/doc/desktop/#DownloadPage/))
* Configuration template file or a valid configuration file

### Configuring GSvar

If you place `cloud_settings_template.ini` file (existing `GSvar.ini` and `settings.ini` should be removed) into the application root folder, an automatic configuration will be triggered and a valid config `settings.ini` will be genereated for your system.

A template file looks very similar to normal configuration file, except all absolute path values are represented as variables.

`[APP_PATH_AUTO]` will generate an absolute path, based on your application root folder, a local operating system will be detected and appropriate slashes will be applied while constructing an absolute path.

`[APP_PATH_UNIX]` does the same, except UNIX style slashes are applied.

All other values in the tamplate file will be transfered as they are.

Here is a small fragment of a template file:

```
curl_ca_bundle=[APP_PATH_AUTO]certificates\\ssl.crt
igv_app=[APP_PATH_AUTO]IGV\\2.17.0\\igv.bat
igv_menu="HGMD\t1\t[APP_PATH_UNIX]igv_tracks/HGMD_PRO_2023_2_fixed.vcf.gz, ClinVar\t1\t[APP_PATH_UNIX]igv_tracks/clinvar_20231121_converted_GRCh38.vcf.gz, RepeatMasker\t0\t[APP_PATH_UNIX]igv_tracks/RepeatMasker_GRCh38.bed.gz, Low confidence regions for SNP calling\t1\t[APP_PATH_UNIX]igv_tracks/low_conf_regions.bed, Copy-number polymorphism regions (AF TruSeqPCRfree)\t1\t[APP_PATH_UNIX]igv_tracks/af_genomes_imgag.igv, Known pathogenic CNVs\t1\t[APP_PATH_UNIX]igv_tracks/cn_pathogenic.bed"

server_host = "localhost"
server_port = 8443
threads = 6
```
`GSvar` app will need to have an access to the [trusted CA](https://en.wikipedia.org/wiki/Certificate_authority). Usually it is a `*.crt` file (see `curl_ca_bundle` in the example above). `libcurl` is responsible for establishing a HTTPS connection.

--

[back to main page](index.md)
