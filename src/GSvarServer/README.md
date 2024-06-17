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
> bin/GSvarServer -p=8443 -i=8080

`p` parameter stands for the HTTPS port number
`i` specifies HTTP port number

GSvar app is needed to utilize the server to its full capacity. However, you can also use any browser to access the data:
> https://[DOMAIN_NAME]:[PORT_NUMBER]


To open the help page, use this address:
> https://[DOMAIN_NAME]:[PORT_NUMBER]/v1/help

## Configuration
The server is configurable via the GSVarServer.ini file located at the `bin/` folder together with all the rest config files.
These are the most important config parameters:
* `ssl_certificate` - location of your SSL certificate
* `ssl_key` - location of your private key
* `server_port` - port used by the server
* `server_host` - domain name used be the server
* `server_root` - folder to be served as static content (any possible file formats)
* `url_lifetime` - lifespan (seconds) of a temporary URL genereated by the server
* `session_duration` - valid period (seconds) of a user session
* `threads` - number of threads used for parallel calculations

These parameters are needed for the server database (stores information about user sessions, temporary URLs, etc.), it is a separate instance of MySQL/MariaDB (not NGSD database)
* gsvar_server_db_host - database host name
* gsvar_server_db_port - database port number
* gsvar_server_db_name - database name
* gsvar_server_db_user - database user name
* gsvar_server_db_pass - database password

## Local development environment
You are going to need a SSL certificate and a key for the server to support HTTPS protocol. For the development purposes self-signed ones will be sufficient:
> openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 -subj "/C=DE/ST=BW/L=Tuebingen/O=test-certificate/CN=localhost" -keyout ~/test-key.key -out ~/test-cert.crt

<strong>NOTE: IGV does not accept self-signed certificates</strong>

GSvarServer requires a MySQL database. You may have your own local instance of MySQL with test data (GSvar does not work with the newer versions):

> docker run --name my-own-mysql -e MYSQL_ROOT_PASSWORD=mypass123 -d mysql:5.7

To start PhpMyAdmin, run this command:

> docker run --name my-own-phpmyadmin -d --link my-own-mysql:db -p 8081:80 phpmyadmin/phpmyadmin

Having a local Apache server instance may be helpful for debugging. To run it in a Docker container, execute the following command (current directory will be
used as a server root):

> docker run -dit --name my-apache-app -p 8080:80 -v "$PWD":/usr/local/apache2/htdocs/ httpd:2.4

## Qt debug statements
Depending on the Qt installation, you may have disabled debug statements by default. To turn them on, follow these steps:
- Open qtlogging.ini in /etc/xdg/QtProject/ (create a new empty file, if it does not exist)
- Add (or modify accordingly) the following config

[Rules]  
*.debug=true  
qt.*.debug=false
