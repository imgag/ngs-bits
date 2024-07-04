# Running GSvar with the server in AWS (Amazon Web Services)

GSvar is a desktop client application (whcih can be built for Windows, Mac, and Linux). It constantly communicates with a HTTP server. The server applicatrion can be deployed (among other places) inside the Amazon Cloud.

## AWS deployment steps

* Create an EC2 Linux instance. We recommed to use Ubuntu or Debian. CPU and RAM parameters may vary significantly, depending on your load. The server can be started even on a minimal instance (however, the compilation process will take a significant amount of time)
* Provide a valid SSL certificate and key for your domain name. Self-signed certificate may be used, but all the functionality based on `htslib` will not be avaliable, since `htslib` will try to validate your certificate and fail.
* Sample data should be stored in a S3 bucket. Use `s3fs` utility to mount a bucket as a regular folder.

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