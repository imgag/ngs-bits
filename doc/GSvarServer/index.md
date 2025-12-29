# GSvarServer
The server handles the interaction between GSvar desktop app and the sample data storage. Having GSvarServer been deployed, we extend the functionality of GSvar: it can now work not only with local data but also with any remote location via HTTPS protocol. The server mimics Apache server's behavour to certain extent. Static content can be served, access can be restricted, connection can be encrypted (SSL certificates), or not (but it is not recommended). However, we have implemented only a small subset of the HTTP specification. The functionality has been reduced exclusively to the needs of GSvar on purpose.

The server handles the interaction between GSvar desktop app and the sample data storage. Having GSvarServer been deployed, we extend the functionality of GSvar: it can now work not only with local data but also with any remote location via HTTPS protocol. The server mimics Apache server's behavour to certain extent. Static content can be served, access can be restricted, connection can be encrypted (SSL certificates), or not (but it is not recommended). However, we have implemented only a small subset of the HTTP specification. The functionality has been reduced exclusively to the needs of GSvar on purpose.

## Dependencies

The server does not add any code dependencies to the project. It as the same dependencies as the rest of ngs-bits.

To allow IGV to read data from the GSvar server, a valid certificate is for the server is needed (a self-signed certificate is not enouth).

The GSvar server needs a own MySQL/MariaDB database for transient data (user sessions, temporary URLs, etc.).

## Build
To build the server, the following steps have to be executed

    > build_3rdparty
    > make build_libs_release
    > build_server_release



## Configuration
The server is configurable via the GSVarServer.ini file located at the `./bin` folder together with all the rest config files.

The configuration of the GSvar server is described in detail [here](configuration.md).

*Note: The server settings are loaded on startup and used from memory (also `megsap_settings_ini`). If you change the settings, you have to restart the server.*

## Running

The following command starts the server (if you are located at the root of the repository):
    
    > ./bin/GSvarServer

You can force the server to ignore the port provided in the config file by using `p` argument and setting your own value:

    > ./bin/GSvarServer -p=8443

GSvar server is intended to be used by the GSvar client app. However, you can also access its Web UI through any browser at:

    https://[HOST_NAME]:[PORT_NUMBER]

It is actually a good way to check, if the server is running. Web UI provides an extensive help page describing all its endpoints:

    https://[HOST_NAME]:[PORT_NUMBER]/help


## FAQ

### How do I start a development instance of the GSvarServer?

Please see [Running a development server](development_instance.md).
