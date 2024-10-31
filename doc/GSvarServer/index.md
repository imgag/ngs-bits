# GSvarServer
The server handles the interaction between GSvar desktop app and the sample data storage. Having GSvarServer been deployed, we extend the functionality of GSvar: it can now work not only with local data but also with any remote location via HTTPS protocol. The server mimics Apache server's behavour to certain extent. Static content can be served, access can be restricted, connection can be encrypted (SSL certificates), or not (but it is not recommended). However, we have implemented only a small subset of the HTTP specification. The functionality has been reduced exclusively to the needs of GSvar on purpose.

The server handles the interaction between GSvar desktop app and the sample data storage. Having GSvarServer been deployed, we extend the functionality of GSvar: it can now work not only with local data but also with any remote location via HTTPS protocol. The server mimics Apache server's behavour to certain extent. Static content can be served, access can be restricted, connection can be encrypted (SSL certificates), or not (but it is not recommended). However, we have implemented only a small subset of the HTTP specification. The functionality has been reduced exclusively to the needs of GSvar on purpose.

## Dependencies

The server does not add any code dependencies to the project. It as the same dependencies as the rest of ngs-bits.

To allow IGV to read data from the GSvar server, a valid certificate is for the server is needed (a self-signed certificate is not enouth).

The GSvar server needs a own MySQL/MariaDB database for transient data (user sessions, temporary URLs, etc.).

## Build
To build the server, the following steps have to be executed
> make build_libs_release
> build_server_release


*Attention: Make sure to compile the [CRYPT_KEY](../GSvar/encrypt_settings.md) into the GSvarServer binary, as it is used for a handshake between client and server.*

## Configuration
The server is configurable via the GSVarServer.ini file located at the `./bin` folder together with all the rest config files.

These are the most important config parameters:
* `ssl_certificate` - location of your SSL certificate
* `ssl_key` - location of your private key
* `server_port` - port used by the server
* `server_host` - domain name used be the server
* `url_lifetime` - lifespan (seconds) of a temporary URL genereated by the server
* `session_duration` - valid period (seconds) of a user session
* `threads` - number of threads used for parallel calculations
* `thread_timeout` - request worker thread timeout in seconds
* `thread_count` - thread pool size of request workers
* `socket_read_timeout` - socket read timeout (seconds)
* `socket_write_timeout` - socket write timeout (seconds)
* `socket_encryption_timeout` - socket encryption wait timeout (seconds)
* `server_root` - root folder used to server static content (used for development only)
* `allow_folder_listing` - enables viewing the list of folder items (used for development only)
* `ngsd_host` - NGSD host name
* `ngsd_port` - NGSD port number
* `ngsd_name` - NGSD database name
* `ngsd_user` - NGSD database user name
* `ngsd_pass` - NGSD user password
* `queue_update_enabled` - turns on SGE update worker (true/false)
* `megsap_settings_ini` - path to the megSAP settings file (additional settings are extracted from this file)

These parameters are needed for the server database (stores information about user sessions, temporary URLs, etc.), it is a separate instance of MySQL/MariaDB (not NGSD database)
* `gsvar_server_db_host` - database host name
* `gsvar_server_db_port` - database port number
* `gsvar_server_db_name` - database name
* `gsvar_server_db_user` - database user name
* `gsvar_server_db_pass` - database password
* `show_raw_request` - flag used for debugging, allows to print out entire HTTP requests in log files(true/false), may significantly increase log sizes, should not be used in production

These are the most important config parameters:

* `ssl_certificate` - location of your SSL certificate
* `ssl_certificate_chain` - If the certificate itself does not contain all information for validation of the certificate, you can provide this optional chertificate chain file. It is used e.g. for Let's encrypt certificates.
* `ssl_key` - location of your private key
* `server_port` - port used by the server
* `server_host` - domain name used be the server
* `url_lifetime` - lifespan (seconds) of a temporary URL genereated by the server
* `session_duration` - valid period (seconds) of a user session
* `threads` - number of threads used for parallel calculations
* `thread_timeout` - request worker thread timeout in seconds
* `thread_count` - thread pool size of request workers
* `socket_read_timeout` - socket read timeout (seconds)
* `socket_write_timeout` - socket write timeout (seconds)
* `socket_encryption_timeout` - socket encryption wait timeout (seconds)
* `server_root` - root folder used to server static content (used for development only)
* `allow_folder_listing` - enables viewing the list of folder items (used for development only)
* `ngsd_host` - NGSD host name
* `ngsd_port` - NGSD port number
* `ngsd_name` - NGSD database name
* `ngsd_user` - NGSD database user name
* `ngsd_pass` - NGSD user password
* `queue_update_enabled` - turns on SGE update worker (true/false)
* `megsap_settings_ini` - path to the megSAP settings file (additional settings are extracted from this file)

These parameters are needed for the server database (stores information about user sessions, temporary URLs, etc.), it is a separate instance of MySQL/MariaDB (not NGSD database)

* `gsvar_server_db_host` - database host name
* `gsvar_server_db_port` - database port number
* `gsvar_server_db_name` - database name
* `gsvar_server_db_user` - database user name
* `gsvar_server_db_pass` - database password
* `show_raw_request` - flag used for debugging, allows to print out entire HTTP requests in log files(true/false), may significantly increase log sizes, should not be used in production

## Running

The following command starts the server (if you are located at the root of the repository):
> ./bin/GSvarServer -p=8443

You can force the server to ignore the port provided in the config file by using `p` argument and setting your own value:
> ./bin/GSvarServer -p=8443

GSvar server is intended to be used by the GSvar client app. However, you can also access its Web UI through any browser at:
> https://[HOST_NAME]:[PORT_NUMBER]

It is actually a good way to check, if the server is running. Web UI also provides an extensive help page describing all its endpoints:
> https://[HOST_NAME]:[PORT_NUMBER]/help


## FAQ

### How do I start a development instance of the GSvarServer?

Please see [Running a development server](development_instance.md).
