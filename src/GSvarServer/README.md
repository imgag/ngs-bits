# GSvarServer
The server handles the interaction between GSvar desktop app and the sample data storage. Having GSvarServer been deployed, we extend the functionality of GSvar: it can now work not only with local data but also with any remote location via HTTPS protocol. The server mimics Apache server's behavour to certain extent. Static content can be served, access can be restricted, connection is encrypted (SSL certificates). However, we have implemented only a small subset of the HTTP specification. The functionality has been reduced exclusively to the needs of GSvar on purpose.

User authentication has been implemented with the help of the standard mechanisms. At the same time we do not use .htaccess files or similar things. Basic HTTP authentication validates user credentials against the exisiting database (the same database GSvar is using). 

## Dependencies
The server does not bring any new dependencies to the project and relies on QT (as the existing code base)

## Build
To build the server, the following steps have to be executed
> make build_libs_release
> build_server_release

## Running
The following command starts the server (if you are located at the root of the repository):
> bin/GSvarServer -p=8443 -l=3
`p` parameter stands for the port number
`l` means the logging detail level:
* 0 only critical and fatal
* 1 += info
* 2 += warning
* 3 += debug

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
* `project_folder` - folder with sample data

## Development
Since the server can work only with HTTPS protocol, you are going to need a SSL certificate and a key. For the development purposes self-signed ones will be sufficient:
> openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 -subj "/C=DE/ST=BW/L=Tuebingen/O=test-certificate/CN=localhost" -keyout ~/test-key.key -out ~/test-cert.crt

NOTE: IGV does not accept self-signed certificates
