# GSvarServer

The server handles the interaction between GSvar desktop app and the sample data storage. Having GSvarServer been deployed, we extend the functionality of GSvar: it can now work not only with local data but also with any remote location via HTTPS protocol. The server mimics Apache server's behavour to certain extent. Static content can be served, access can be restricted, connection can be encrypted (SSL certificates), or not (but it is not recommended). However, we have implemented only a small subset of the HTTP specification. The functionality has been reduced exclusively to the needs of GSvar on purpose.

The server handles the interaction between GSvar desktop app and the sample data storage. Having GSvarServer been deployed, we extend the functionality of GSvar: it can now work not only with local data but also with any remote location via HTTPS protocol. The server mimics Apache server's behavour to certain extent. Static content can be served, access can be restricted, connection can be encrypted (SSL certificates), or not (but it is not recommended). However, we have implemented only a small subset of the HTTP specification. The functionality has been reduced exclusively to the needs of GSvar on purpose.

## Dependencies

The server does not add any code dependencies to the project. It uses only internal ngs-bits libraries. The server can be deployed on Linux and Mac, Windows is not supported.

To allow IGV to read data from the GSvar server, a valid certificate is for the server is needed (a self-signed certificate is not enouth).

The GSvar server needs a own MySQL/MariaDB database for transient data (user sessions, temporary URLs, etc.).

## Build

To build the server, the following steps have to be executed

    > build_3rdparty make build_libs_release build_server_release

## Configuration

The server is configurable via the GSVarServer.ini file located at the `./bin` folder together with all the rest config files.

The configuration of the GSvar server is described in detail [here](configuration.md).

*Note: The server settings are loaded on startup and used from memory (also `megsap_settings_ini`). If you change the settings, you have to restart the server.*

## Running

The following command starts the server (if you are located at the root of the repository):

    > ./bin/GSvarServer

You can make the server ignore the port parameter from the config file by using `p` argument and setting your own value:

    > ./bin/GSvarServer -p=8443

GSvar server is intended to be used by the GSvar client app. However, you can also access its Web UI through any browser at:

    https://[HOST_NAME]:[PORT_NUMBER]

It is actually a good way to check, if the server is running. Web UI provides an extensive help page describing all its endpoints:

    https://[HOST_NAME]:[PORT_NUMBER]/help

## FAQ

### How to update an existing server instance to the latest version?

1. Get the latest changes from the github repository:

        > make pull

2. Stop the running instance

3. If you changed the server database schema, you should increase `EXPECTED_SCHEMA_VERSION` value in `ServerDB.h` by 1. When the schema version chganges, the server database will be removed and recreated during the first launch. All the user sessions and URLs will be lost, users will have to relogin. NGSD database remains intact.

4. Build the new version:

        > make build_3rdparty build_libs_release build_server_release

5. Check for the NGSD database schema changes: outdated database may cause serious problems i.e. unstable client or server, freezes, crashes. Open `GSvar` app, go to `NGSD` -> `Admin` -> `Maintanance` menu, select `Compare structure of test and production` and press `Execute`. Soon you will see what has been changed in the database schema. You will have to adjust your database accordingly. We recommend to make a backup before modifying the database.

6. The the new version of the server is reday to be used now.

### How do I start a development instance of the GSvarServer?

Please see [Running a development server](development/development_instance.md).

### Is it possible to commuincate with the queuing engine via some HTTP API?

Yes, asolutely. To learn how to do it, please read the corresponding section of this [documentation](qe_api.md).

### It looks like the server uses HTTPS. How do I deal with the SSL certificates?

Yes, `GSvarServer` supports only HTTPS protocol and you will need to configure the server to use a SSL certificate. There are multiple ways to get one, we recommend acquiring certificates through [Let's Encrypt](https://letsencrypt.org/getting-started/)

1. Install `certbot`:

        > sudo apt update
        > sudo apt install -y certbot

2. Generate a certificate:

        > sudo certbot certonly --standalone --non-interactive --agree-tos --email [YOUR_EMAIL] -d [YOUR_DOMAIN_NAME]
        
3. Pay attention to the command output and adjust the server config accordingly:

        ssl_certificate = "/etc/letsencrypt/live/[YOUR_DOMAIN_NAME]/cert.pem"
        ssl_key = "/etc/letsencrypt/live/[YOUR_DOMAIN_NAME]/privkey.pem"
        ssl_certificate_chain = "/etc/letsencrypt/live/[YOUR_DOMAIN_NAME]/fullchain.pem"

### What are the minimal system requirements for building and running the server?

The server can be built and started on Linux and Mac, Windows is not supported. Current version requires about 16GB of RAM and at least 2 core CPU for the compilation. For running the server, however, you may need significanlty less RAM - about 4GB should be sufficient. Memory consumption mainly depends on how many active users and active requests the server is processing simultaneoulsy.

### Do I need to download, install, and configure BLAT server manually?

No, you just need to specify the port number BLAT server should be running on in the server config file. `GSvarServer` will donwload and start BLAT server automatically. It will also turn BLAT server off, when `GSvarServer` is being stopped.

### How to configure a queuing engine?

Two qeueuing engines are supported natively: [SGE](install_sge.md) and [Slurm](install_slurm.md).  
Additionally, a web-service can be used for other queuing engines or custom analysis queuing: more information on how it works can be found [here](qe_api.md)

### Is it possible to deploy GSvarServer in a cloud (e.g. in AWS or Hetzner)?

Yes, more information can be found [here](run_gsvar_in_cloud.md)

### How to test if the server is working correctly?

* The easiest way is to open a browser at `https://[DOMAIN_NAME]:[SERVER_PORT]` (use `localhost`, if the server is running locally). You should be able to see the index page of `GSvarServer`. 

* To make sure the client-server communication works properly, you need to login inside your `GSvar` application. You should be able to see the login winodw, and logging in should work with valid credentials.

* Reading BAM/CRAM files sometimes does not work properly (due to SSL problems, missing or incorrect `htslib` dependencies, etc.). Try running `Tools` -> `Determine gender` -> `Based on heterozygous SNPs on X` agains a known sample. If the tools returns valid results, everything is fine.

* Streaming of GZ files may not work properly. Try running `Tools` -> `Sample ancesntry` against a known sample. If the tools returns valid results, everything is fine.

* In `Help` -> `About` dialog you can find more details about the server and the client you are currently using. This information may be helpful in troubleshooting.
