# GSvarServer
The server handles the interaction between GSvar desktop app and the sample data storage. Having GSvarServer been deployed, we extend the functionality of GSvar: it can now work not only with local data but also with any remote location via HTTPS protocol. The server mimics Apache server's behavour to certain extent. Static content can be served, access can be restricted, connection can be encrypted (SSL certificates), or not (but it is not recommended). However, we have implemented only a small subset of the HTTP specification. The functionality has been reduced exclusively to the needs of GSvar on purpose.

## Dependencies
The server does not bring any new dependencies to the project and relies on QT (as the existing code base)

## Build
To build the server, the following steps have to be executed
> make build_libs_release
> build_server_release

## Running
The following command starts the server (if you are located at the root of the repository):
> ./bin/GSvarServer -p=8443

You can force the server to ignore the port provided in the config file by using `p` argument and setting your own value:
> ./bin/GSvarServer -p=8443

GSvar server is intended to be used by the GSvar client app. However, you can also access its Web UI through any browser at:
> https://[HOST_NAME]:[PORT_NUMBER]

It is actually a good way to check, if the server is running. Web UI also provides an extensive help page describing all its endpoints:
> https://[HOST_NAME]:[PORT_NUMBER]/help

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
* `server_root` - root folder used to server static content (used for tests)
* `allow_folder_listing` - enables viewing the list of folder items (true/false), currently not used
* `ngsd_enabled` - allows to use NGSD database (true/false)
* `ngsd_host` - NGSD host name
* `ngsd_port` - NGSD port number
* `ngsd_name` - NGSD database name
* `ngsd_user` - NGSD database user name
* `ngsd_pass` - NGSD user password
* `queue_update_enabled` - turns on SGE update worker (true/false)
* `megsap_settings_ini` - path to the megSAP settings file

These parameters are needed for the server database (stores information about user sessions, temporary URLs, etc.), it is a separate instance of MySQL/MariaDB (not NGSD database)
* `gsvar_server_db_host` - database host name
* `gsvar_server_db_port` - database port number
* `gsvar_server_db_name` - database name
* `gsvar_server_db_user` - database user name
* `gsvar_server_db_pass` - database password
* `show_raw_request` - flag used for debugging, allows to print out entire HTTP requests in log files(true/false), may significantly increase log sizes, should not be used in production

## Local development environment
You are going to need a SSL certificate and a key for the server to support HTTPS protocol. For the development purposes self-signed ones will be sufficient:
> openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 -subj "/C=DE/ST=BW/L=Tuebingen/O=test-certificate/CN=localhost" -keyout ~/test-key.key -out ~/test-cert.crt

<strong>NOTE: IGV does not accept self-signed certificates</strong>

GSvarServer requires a MySQL/MariaDB database. You may have your own local instance of MySQL with test data:

> docker run --name my-own-mysql -e MYSQL_ROOT_PASSWORD=mypass123 -d mysql:5.7

To start PhpMyAdmin, run this command:

> docker run --name my-own-phpmyadmin -d --link my-own-mysql:db -p 8081:80 phpmyadmin/phpmyadmin


## Qt debug statements
Depending on the Qt installation, you may have disabled debug statements by default. To turn them on, follow these steps:
- Open qtlogging.ini in /etc/xdg/QtProject/ (create a new empty file, if it does not exist)
- Add (or modify accordingly) the following config

[Rules]  
*.debug=true  
qt.*.debug=false

## Deploying a production instance
* SSH to the server
* Build the server binary by following instructions from [here](https://github.com/imgag/ngs-bits/blob/master/doc/install_unix.md) (do not forget to set the encryption key at `./src/cppCORE/CRYPT_KEY.txt`)
* Run`make deploy_server_nobuild` to copy the server files and configs to the correct location (handled by Makefile)
* Start the server: `sudo /usr/sbin/service gsvar start`
* Check the server status: `sudo /usr/sbin/service gsvar status`
* Get the latest `N` log records: `sudo journalctl -u gsvar.service -n [N]`


## Running a development server on a (local) machine

### Setting up a GSvar server for testing (on Ubuntu)

First you need a unused port on the development server. You can either select a random 5-digit port and check with `netstat -lntu` if the port is already used.
Or you can use the following 1-liner to get a free port from the kernel:

    > python -c 'import socket; s=socket.socket(); s.bind(("", 0)); print(s.getsockname()[1]); s.close()' 

Additionally you have to provide a certificate. If you do not have a valid certificate for your machine you can create a self-signed one:

    > openssl genrsa -out GSvarServer.key 2048
    > openssl req -new -key GSvarServer.key -out GSvarServer.csr
    > openssl x509 -signkey GSvarServer.key -in GSvarServer.csr -req -days 365 -out GSvarServer.crt

If you use self-signed certificates IGV and libcurl will not work.(See possible fix [below](#trust-self-signed-certificates-on-ubuntu))

Next you have to create a `GSvarServer.ini` in the `bin`folder:

    > cp bin/GSvarServer.ini.example bin/GSvarServer.ini

And fill in all required settings (like port, NGSD credentials, sample paths, ...). If the server runs on a different machine than the client you have to use the hostname/dns name of the server for `server_host`.

If you use encrypted passwords you have to add the crypt key to the `cppCORE` directory (sometimes not working on linux) or directly to the `cppCORE.pro` file. 

Next step is to build the server:

    > make build_libs_release build_server_release

And run it:

    > ./bin/GSvarServer

Now you can adapt the settings in your client and connect to the server.


### Trust self-signed certificates on Ubuntu
For the development and testing purposes it is possible to run a local instance of GSvar Server. However, if you are using self-signed certificates, you will have to make them trusted (otherwise IGV and libcurl will not be able to verify them):

Install CA certificates package:

    > sudo apt-get install ca-certificates

Copy your self-signed certificate to this location:

    > sudo cp YOUR_CERTIFICATE.crt /usr/local/share/ca-certificates

Update the list of certificate authorities:

    > sudo update-ca-certificates
